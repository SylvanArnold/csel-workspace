// skeleton.c
#include <linux/cdev.h>        /* needed for char device driver */
#include <linux/fs.h>          /* needed for device drivers */
#include <linux/init.h>        /* needed for macros */
#include <linux/kernel.h>      /* needed for debugging */
#include <linux/module.h>      /* needed by all modules */
#include <linux/moduleparam.h> /* needed for module parameters */
#include <linux/device.h>      /* needed for class/device creation */
#include <linux/string.h>      /* needed for strnlen */
#include <linux/uaccess.h>     /* needed to copy data to/from user */

#include <linux/slab.h> // needed for kmalloc and kfree

// Device structure to hold device informations
struct skeleton_device {
    char name[20];
    dev_t dev_number;
    struct cdev cdev;
};

static struct skeleton_device *skeleton_devices = NULL;
static char **skeleton_buffers = NULL;
static struct class *skeleton_class = NULL;

#define SKELETON_BUFFER_SIZE 256


static int devices_number = 1; // default device number is 1, can be set at module load time
module_param(devices_number, int, 0);


static int skeleton_open(struct inode* i, struct file* f)
{
    int idx;

    pr_info("skeleton : open operation... major:%d, minor:%d\n",
            imajor(i),
            iminor(i));

    idx = iminor(i) - MINOR(skeleton_devices[0].dev_number);
    if (idx < 0 || idx >= devices_number)
        return -ENODEV;

    // store the internal buffer pointer in the file's private data for later use in read/write operations
    f->private_data = skeleton_buffers[idx];

    if ((f->f_mode & (FMODE_READ | FMODE_WRITE)) == (FMODE_READ | FMODE_WRITE)) {
        pr_info("skeleton : opened for reading & writing...\n");
    } else if ((f->f_mode & FMODE_READ) != 0) {
        pr_info("skeleton : opened for reading...\n");
    } else if ((f->f_mode & FMODE_WRITE) != 0) {
        pr_info("skeleton : opened for writing...\n");
    }
    return 0;
}

static int skeleton_release(struct inode* i, struct file* f)
{
    pr_info("skeleton: release operation...\n");
    return 0;
}

// Read data from the internal buffer
// Caller tell how many bytes he wants to read and the offset in the buffer, we copy the data to user space and update the offset
static ssize_t skeleton_read(struct file* f,char __user* buf,size_t count, loff_t* off)
{
    char *data = (char*)f-> private_data; // get the internal buffer from the file's private data
    size_t len;
    if (data == NULL)
        return 0; // EOF

    len = strnlen(data, SKELETON_BUFFER_SIZE);

    // check if offset is beyond the end of the buffer (means no more data to read)
    if (*off >= len) 
        return 0; // EOF

    // check if the requested count exceeds the remaining data,
    // if so clamp it to the remaining data
    if (count > len - *off)
        count = len - *off;

    // copy data from internal buffer to user space
    if (copy_to_user(buf, data + *off, count))
        return -EFAULT;

    *off += count;
    return count; // return the number of bytes successfully read
}

// Write data to the internal buffer
// Data is copied into a fixed-size per-device buffer allocated at module init.
static ssize_t skeleton_write(struct file* f,const char __user* buf,size_t count,loff_t* off)
{
    char *data = (char*)f-> private_data; // get the internal buffer from the file's private data
    size_t bytes_to_copy;

    if (!data)
        return -ENODEV;

    bytes_to_copy = min(count, (size_t)SKELETON_BUFFER_SIZE - 1);

    // copy data from user space to internal buffer
    if (copy_from_user(data, buf, bytes_to_copy))
        return -EFAULT;

    data[bytes_to_copy] = '\0'; // null terminate the string
    *off = 0; // reset read position so next read starts from beginning
    return bytes_to_copy;
}

static struct file_operations skeleton_fops = {
    .owner   = THIS_MODULE,
    .open    = skeleton_open,
    .read    = skeleton_read,
    .write   = skeleton_write,
    .release = skeleton_release,
};

static int __init skeleton_init(void)
{
    int status;
    int i, j;

    if (devices_number <= 0)
        return -EINVAL;

    
    skeleton_devices = kmalloc_array(devices_number, sizeof(struct skeleton_device), GFP_KERNEL);
    if (!skeleton_devices){
        pr_err("skeleton: failed to allocate memory for devices\n");
        return -ENOMEM;
    }

    skeleton_buffers = kmalloc_array(devices_number, sizeof(char *), GFP_KERNEL);
    if (!skeleton_buffers) {
        pr_err("skeleton: failed to allocate memory for buffers\n");
        kfree(skeleton_devices);
        skeleton_devices = NULL;
        return -ENOMEM;
    }

    for (i = 0; i < devices_number; i++) {
        skeleton_buffers[i] = kmalloc(SKELETON_BUFFER_SIZE, GFP_KERNEL);
        if (!skeleton_buffers[i]) {
            pr_err("skeleton: failed to allocate buffer for device %d\n", i);
            for (j = 0; j < i; j++)
                kfree(skeleton_buffers[j]);
            kfree(skeleton_buffers);
            skeleton_buffers = NULL;
            kfree(skeleton_devices);
            skeleton_devices = NULL;
            return -ENOMEM;
        }
        skeleton_buffers[i][0] = '\0';
    }

    // allocate a char device region for all devices
    status = alloc_chrdev_region(&skeleton_devices[0].dev_number, 0, devices_number, "skeleton");
    if (status < 0) {
        pr_err("skeleton: failed to allocate char device region\n");
        for (i = 0; i < devices_number; i++)
            kfree(skeleton_buffers[i]);
        kfree(skeleton_buffers);
        skeleton_buffers = NULL;
        kfree(skeleton_devices);
        skeleton_devices = NULL;
        return status;
    }

    skeleton_class = class_create(THIS_MODULE, "skeleton");
    if (IS_ERR(skeleton_class)) {
        status = PTR_ERR(skeleton_class);
        skeleton_class = NULL;
        unregister_chrdev_region(skeleton_devices[0].dev_number, devices_number);
        for (i = 0; i < devices_number; i++)
            kfree(skeleton_buffers[i]);
        kfree(skeleton_buffers);
        skeleton_buffers = NULL;
        kfree(skeleton_devices);
        skeleton_devices = NULL;
        return status;
    }

    for (i = 0; i < devices_number; i++) { // initialize each device
        snprintf(skeleton_devices[i].name, sizeof(skeleton_devices[i].name), "skeleton%d", i); // set the device name
        skeleton_devices[i].dev_number = MKDEV(MAJOR(skeleton_devices[0].dev_number), MINOR(skeleton_devices[0].dev_number) + i); 
        cdev_init(&skeleton_devices[i].cdev, &skeleton_fops); 
        skeleton_devices[i].cdev.owner = THIS_MODULE;
        status = cdev_add(&skeleton_devices[i].cdev, skeleton_devices[i].dev_number, 1);
        if (status) {
            pr_err("skeleton: failed to add cdev for device %d\n", i);
            // cleanup already added devices
            for (j = 0; j < i; j++) {
                cdev_del(&skeleton_devices[j].cdev);
            }
            unregister_chrdev_region(skeleton_devices[0].dev_number, devices_number);
            for (j = 0; j < devices_number; j++)
                kfree(skeleton_buffers[j]);
            kfree(skeleton_buffers);
            skeleton_buffers = NULL;
            kfree(skeleton_devices);
            skeleton_devices = NULL;
            class_destroy(skeleton_class);
            skeleton_class = NULL;
            return status;
        }

        if (IS_ERR(device_create(skeleton_class,
                                 NULL,
                                 skeleton_devices[i].dev_number,
                                 NULL,
                                 "%s",
                                 skeleton_devices[i].name))) {
            pr_err("skeleton: failed to create device node for device %d\n", i);
            cdev_del(&skeleton_devices[i].cdev);
            for (j = 0; j < i; j++) {
                device_destroy(skeleton_class, skeleton_devices[j].dev_number);
                cdev_del(&skeleton_devices[j].cdev);
            }
            class_destroy(skeleton_class);
            skeleton_class = NULL;
            unregister_chrdev_region(skeleton_devices[0].dev_number, devices_number);
            for (j = 0; j < devices_number; j++)
                kfree(skeleton_buffers[j]);
            kfree(skeleton_buffers);
            skeleton_buffers = NULL;
            kfree(skeleton_devices);
            skeleton_devices = NULL;
            return -ENOMEM;
        }
    }

    pr_info("skeleton: allocated char device region with major %d and %d devices\n", MAJOR(skeleton_devices[0].dev_number), devices_number);
    pr_info ("Driver module loaded\n");
    return 0;
}

static void __exit skeleton_exit(void)
{
    int i;

    if (skeleton_class && skeleton_devices) {
        for (i = 0; i < devices_number; i++)
            device_destroy(skeleton_class, skeleton_devices[i].dev_number);
        class_destroy(skeleton_class);
        skeleton_class = NULL;
    }

    if (skeleton_devices) {
        for (i = 0; i < devices_number; i++) {
            cdev_del(&skeleton_devices[i].cdev);
        }
        unregister_chrdev_region(skeleton_devices[0].dev_number, devices_number);
        kfree(skeleton_devices);
        skeleton_devices = NULL;
    }

    if (skeleton_buffers) {
        for (i = 0; i < devices_number; i++)
            kfree(skeleton_buffers[i]);
        kfree(skeleton_buffers);
        skeleton_buffers = NULL;
    }

    pr_info ("Driver module unloaded\n");
}

module_init (skeleton_init);
module_exit (skeleton_exit);

MODULE_AUTHOR ("Sylvan Arnold <sylvan.arnold@hes-so.ch>");
MODULE_DESCRIPTION ("Driver module");
MODULE_LICENSE ("GPL");