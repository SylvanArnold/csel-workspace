/* skeleton.c */
#include <linux/init.h>   /* needed for macros */
#include <linux/kernel.h> /* needed for debugging */
#include <linux/module.h> /* needed by all modules */

#include <linux/cdev.h>    /* needed for char device driver */
#include <linux/fs.h>      /* needed for device drivers */
#include <linux/uaccess.h> /* needed to copy data to/from user */

#include <linux/ioctl.h> /* needed for ioctl handling */
#include "skeleton.h"

static struct skeleton_config config;
static int val;

static long skeleton_ioctl(struct file* f, unsigned int cmd, unsigned long arg)
{
    int status = 0;
    switch (cmd) {
        case SKELETON_IO_RESET:
            val = 0;
            memset(&config, 0, sizeof(config));
            pr_info("skeleton-ioctl: reset command\n");
            break;

        case SKELETON_IO_WR_REF:
            if (_IOC_SIZE(cmd) == sizeof(config))
                status =
                    copy_from_user(&config, (char __user*)arg, _IOC_SIZE(cmd));
            else
                status = -EFAULT;
            pr_info("skeleton-ioctl: write config\n");
            break;

        case SKELETON_IO_RD_REF:
            if (_IOC_SIZE(cmd) == sizeof(config))
                status =
                    copy_to_user((char __user*)arg, &config, _IOC_SIZE(cmd));
            else
                status = -EFAULT;
            pr_info("skeleton-ioctl: read config\n");
            break;

        case SKELETON_IO_WR_VAL:
            val = arg;
            pr_info("skeleton-ioctl: write value=%d\n", val);
            break;

        case SKELETON_IO_RD_VAL:
            status = val;
            pr_info("skeleton-ioctl: read value=%d\n", status);
            break;

        default:
            pr_info("skeleton-ioctl: unknown command (cmd=%d)\n", cmd);
            status = -EFAULT;
            break;
    }

    return status;
}

static struct file_operations skeleton_fops = {
    .owner          = THIS_MODULE,
    .unlocked_ioctl = skeleton_ioctl,
};

static dev_t skeleton_dev;
static struct cdev skeleton_cdev;

static int __init skeleton_init(void)
{
    int status;

    status = alloc_chrdev_region(&skeleton_dev, 0, 1, "mymodule");
    if (status == 0) {
        cdev_init(&skeleton_cdev, &skeleton_fops);
        skeleton_cdev.owner = THIS_MODULE;
        status              = cdev_add(&skeleton_cdev, skeleton_dev, 1);
    }

    pr_info("Linux module skeleton loaded\n");
    return 0;
}

static void __exit skeleton_exit(void)
{
    cdev_del(&skeleton_cdev);
    unregister_chrdev_region(skeleton_dev, 1);

    pr_info("Linux module skeleton unloaded\n");
}

module_init(skeleton_init);
module_exit(skeleton_exit);

MODULE_AUTHOR("Daniel Gachet <daniel.gachet@hefr.ch>");
MODULE_DESCRIPTION("Module skeleton");
MODULE_LICENSE("GPL");
