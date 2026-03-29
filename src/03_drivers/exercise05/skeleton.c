/* skeleton.c */
#include <linux/init.h>   /* needed for macros */
#include <linux/kernel.h> /* needed for debugging */
#include <linux/module.h> /* needed by all modules */

#include <linux/cdev.h>    /* needed for char device driver */
#include <linux/fs.h>      /* needed for device drivers */
#include <linux/uaccess.h> /* needed to copy data to/from user */
#include <linux/slab.h>    /* needed for kmalloc/kfree */

#include <linux/device.h> /* needed for sysfs handling */
#include <linux/platform_device.h> /* needed for sysfs handling */

struct skeleton_config {
    int id;
    long ref;
    char name[30];
    char descr[30];
};

static struct skeleton_config config;

// Needed for char device
static struct cdev cdev;
static dev_t dev_number;
static char * chardev_data = NULL; 


static int sysfs_data;

ssize_t sysfs_show_val(struct device* dev, struct device_attribute* attr, char* buf) {
    sprintf(buf, "%d\n", sysfs_data);
    return strlen(buf);
}
ssize_t sysfs_store_val(struct device* dev, struct device_attribute* attr, const char* buf, size_t count){
    sysfs_data = simple_strtol(buf, 0, 10);
    return count;
}

// Expose the "sysfs_data" attribute in sysfs with read/write permissions and the corresponding show/store functions
DEVICE_ATTR(sysfs_data, 0664, sysfs_show_val, sysfs_store_val);

// show the content of the "config" structure in sysfs
ssize_t sysfs_show_cfg(struct device* dev, struct device_attribute* attr, char* buf) {
    sprintf(buf,
            "%d %ld %s %s\n",
            config.id,
            config.ref,
            config.name,
            config.descr);
    return strlen(buf);
}

// store the content of the "config" structure in sysfs
ssize_t sysfs_store_cfg(struct device* dev, struct device_attribute* attr, const char* buf, size_t count)
{
    sscanf(buf,
           "%d %ld %s %s",
           &config.id,
           &config.ref,
           config.name,
           config.descr);
    return count;
}

// Expose the "config" attribute in sysfs with read/write permissions and the corresponding show/store functions
DEVICE_ATTR(cfg, 0664, sysfs_show_cfg, sysfs_store_cfg);


static int chardev_open(struct inode* i, struct file* f)
{
    pr_info("skeleton : open operation... major:%d, minor:%d\n",
            imajor(i),
            iminor(i));
    if ((f->f_mode & (FMODE_READ | FMODE_WRITE)) != 0) {
        pr_info("skeleton : opened for reading & writing...\n");
    } else if ((f->f_mode & FMODE_READ) != 0) {
        pr_info("skeleton : opened for reading...\n");
    } else if ((f->f_mode & FMODE_WRITE) != 0) {
        pr_info("skeleton : opened for writing...\n");
    }
    return 0;
}

static int chardev_release(struct inode* i, struct file* f)
{
    pr_info("skeleton: release operation...\n");
    return 0;
}

// Read chardev_data from the internal buffer
// Caller tell how many bytes he wants to read and the offset in the buffer, we copy the data to user space and update the offset
static ssize_t chardev_read(struct file* f,char __user* buf,size_t count, loff_t* off)
{
    size_t len;
    if (chardev_data == NULL)
        return 0; // EOF

    len = strlen(chardev_data);

    // check if offset is beyond the end of the buffer (means no more data to read)
    if (*off >= len) 
        return 0; // EOF

    // check if the requested count exceeds the remaining data,
    // if so clamp it to the remaining data
    if (count > len - *off)
        count = len - *off;

    // copy chardev_data from internal buffer to user space
    if (copy_to_user(buf, chardev_data + *off, count))
        return -EFAULT;

    *off += count;
    return count; // return the number of bytes successfully read
}

// Write chardev_data to the internal buffer
// We free the previous buffer if any, allocate a new one to hold the incoming data,
static ssize_t chardev_write(struct file* f,const char __user* buf,size_t count,loff_t* off)
{
    // free previous chardev_data if any
    if (chardev_data) {
        kfree(chardev_data);
        chardev_data = NULL;
    }
    // allocate new buffer to hold the incoming data, add 1 for null terminator
    chardev_data = kmalloc(count + 1, GFP_KERNEL);
    if (!chardev_data)
        return -ENOMEM;

    // copy chardev_data from user space to internal buffer
    if (copy_from_user(chardev_data, buf, count))
        return -EFAULT;

    chardev_data[count] = '\0'; // null terminate the string
    return count;
}

static struct file_operations fops = {
    .owner   = THIS_MODULE,
    .open    = chardev_open,
    .read    = chardev_read,
    .write   = chardev_write,
    .release = chardev_release,
};

static struct class* device_class;
static struct device* sysfs_device;

static int __init skeleton_init(void) {
    int status = alloc_chrdev_region(&dev_number, 0, 1, "skeleton");

    if (status == 0) {
        cdev_init(&cdev, &fops); // initialize the char device with the file operations
        cdev.owner = THIS_MODULE; // set the owner of the device to this module
        status = cdev_add(&cdev, dev_number, 1); // add the char device to the system
    }

    if (status == 0) {
        device_class = class_create(THIS_MODULE, "ex5_device_class"); // Create a class in sysfs
        if (IS_ERR(device_class)) {
            status = PTR_ERR(device_class);
            device_class = NULL;
        }
    }

    if (status == 0) {
        sysfs_device = device_create(device_class, NULL, dev_number, NULL, "ex5_sysfs_device"); // Create a device in the class
        if (IS_ERR(sysfs_device)) {
            status = PTR_ERR(sysfs_device);
            sysfs_device = NULL;
        }
    }

    if (status == 0) {
        // Create the sys files for the "data" and "config" attributes
        // these dev_attr_sysfs_data and dev_attr_cfg are created by the DEVICE_ATTR macros
        status = device_create_file(sysfs_device, &dev_attr_sysfs_data);
        if (status == 0)
            status = device_create_file(sysfs_device, &dev_attr_cfg);
    }

    if (status != 0) {
        if (sysfs_device) {
            device_remove_file(sysfs_device, &dev_attr_sysfs_data);
            device_remove_file(sysfs_device, &dev_attr_cfg);
            device_destroy(device_class, dev_number);
            sysfs_device = NULL;
        }
        if (device_class) {
            class_destroy(device_class);
            device_class = NULL;
        }
        cdev_del(&cdev); // remove the char device from the system
        unregister_chrdev_region(dev_number, 1); // release the allocated device region
        return status;
    }

    pr_info ("Linux module skeleton loaded\n");
    return 0;
}

static void __exit skeleton_exit(void)
{

    // release the allocated buffer if any
    if (chardev_data) {
        kfree(chardev_data);
        chardev_data = NULL;
    }
    // unregister the char device region
    cdev_del(&cdev); // remove the char device from the system
    unregister_chrdev_region(dev_number, 1); // release the allocated device region


    // remove the files and destroy the device and class
    if (sysfs_device) {
        device_remove_file(sysfs_device, &dev_attr_sysfs_data);
        device_remove_file(sysfs_device, &dev_attr_cfg);
        device_destroy(device_class, dev_number);
    }
    if (device_class)
        class_destroy(device_class);

    pr_info ("Linux module skeleton unloaded\n");
}

module_init (skeleton_init);
module_exit (skeleton_exit);

MODULE_AUTHOR ("Sylvan Arnold <sylvan.arnold@hes-so.ch>");
MODULE_DESCRIPTION ("Module skeleton");
MODULE_LICENSE ("GPL");