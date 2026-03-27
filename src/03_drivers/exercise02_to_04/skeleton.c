// skeleton.c
#include <linux/cdev.h>        /* needed for char device driver */
#include <linux/fs.h>          /* needed for device drivers */
#include <linux/init.h>        /* needed for macros */
#include <linux/kernel.h>      /* needed for debugging */
#include <linux/module.h>      /* needed by all modules */
#include <linux/moduleparam.h> /* needed for module parameters */
#include <linux/uaccess.h>     /* needed to copy data to/from user */

#include <linux/slab.h> // needed for kmalloc and kfree


static char * data = NULL;
static dev_t skeleton_dev_number;
static struct cdev skeleton_cdev;

static int devices_number = 1;
module_param(devices_number, int, 0);


static int skeleton_open(struct inode* i, struct file* f)
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

static int skeleton_release(struct inode* i, struct file* f)
{
    pr_info("skeleton: release operation...\n");
    return 0;
}

// Read data from the internal buffer
// Caller tell how many bytes he wants to read and the offset in the buffer, we copy the data to user space and update the offset
static ssize_t skeleton_read(struct file* f,char __user* buf,size_t count, loff_t* off)
{
    size_t len;
    if (data == NULL)
        return 0; // EOF

    len = strlen(data);

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
// We free the previous buffer if any, allocate a new one to hold the incoming data,
static ssize_t skeleton_write(struct file* f,const char __user* buf,size_t count,loff_t* off)
{
    // free previous data if any
    if (data) {
        kfree(data);
        data = NULL;
    }
    // allocate new buffer to hold the incoming data, add 1 for null terminator
    data = kmalloc(count + 1, GFP_KERNEL);
    if (!data)
        return -ENOMEM;

    // copy data from user space to internal buffer
    if (copy_from_user(data, buf, count))
        return -EFAULT;

    data[count] = '\0'; // null terminate the string
    return count;
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
    // allocate a char device region with 1 minor number, the major number will be dynamically allocated by the kernel
    int status = alloc_chrdev_region(&skeleton_dev_number, 0, 1, "skeleton");
    if (status == 0) {
            cdev_init(&skeleton_cdev, &skeleton_fops); // initialize the char device with the file operations
            skeleton_cdev.owner = THIS_MODULE; // set the owner of the device to this module
            status= cdev_add(&skeleton_cdev, skeleton_dev_number, 1); // add the char device to the system
        }
    
    pr_info("skeleton: allocated char device region with major %d and minor %d\n", MAJOR(skeleton_dev_number), MINOR(skeleton_dev_number));
    pr_info ("Driver module loaded\n");
    return 0;
}

static void __exit skeleton_exit(void)
{
    // release the allocated buffer if any
    if (data) {
        kfree(data);
        data = NULL;
    }
    // unregister the char device region
    cdev_del(&skeleton_cdev); // remove the char device from the system
    unregister_chrdev_region(skeleton_dev_number, 1); // release the allocated device region

    pr_info ("Driver module unloaded\n");
}

module_init (skeleton_init);
module_exit (skeleton_exit);

MODULE_AUTHOR ("Sylvan Arnold <sylvan.arnold@hes-so.ch>");
MODULE_DESCRIPTION ("Driver module");
MODULE_LICENSE ("GPL");