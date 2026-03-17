/* skeleton.c */
#include <linux/init.h>   /* needed for macros */
#include <linux/kernel.h> /* needed for debugging */
#include <linux/module.h> /* needed by all modules */

#include <linux/cdev.h>    /* needed for char device driver */
#include <linux/fs.h>      /* needed for device drivers */
#include <linux/uaccess.h> /* needed to copy data to/from user */

#include <linux/device.h> /* needed for sysfs handling */
#include <linux/miscdevice.h>
#include <linux/of.h>
#include <linux/platform_device.h> /* needed for sysfs handling */

static int skeleton_open(struct inode* i, struct file* f)
{
    pr_info("skeleton : open operation... major:%d, minor:%d\n",
            imajor(i),
            iminor(i));
    return 0;
}

static int skeleton_release(struct inode* i, struct file* f)
{
    pr_info("skeleton: release operation...\n");
    return 0;
}

static ssize_t skeleton_read(struct file* f,
                             char __user* buf,
                             size_t count,
                             loff_t* off)
{
    pr_info("skeleton: read operation... read=%ld\n", count);
    return 0;
}

static ssize_t skeleton_write(struct file* f,
                              const char __user* buf,
                              size_t count,
                              loff_t* off)
{
    pr_info("skeleton: write operation... written=%ld\n", count);
    return count;
}

static struct file_operations fops = {
    .owner   = THIS_MODULE,
    .open    = skeleton_open,
    .read    = skeleton_read,
    .write   = skeleton_write,
    .release = skeleton_release,
};

struct miscdevice misc_device = {
    .minor = MISC_DYNAMIC_MINOR,
    .fops  = &fops,
    .name  = "mydevice",
    .mode  = 0777,
};

int skeleton_probe(struct platform_device* pdev)
{
    pr_info("skeleton - driver probe %llx (%s)- M.m=%d.%d\n",
            (unsigned long long)pdev,
            pdev->name,
            MAJOR(pdev->dev.devt),
            MINOR(pdev->dev.devt));

    // register misc device ...
    int ret = misc_register(&misc_device);

    // read device tree attribute
    struct device_node* dt_node = pdev->dev.of_node;
    const char* prop            = 0;
    if (dt_node)
        ret = of_property_read_string(dt_node, "attribute", &prop);
    else
        pr_info("mymodule not found!\n");
    if ((prop != 0) && (ret == 0))
        pr_info("attribute=%s (ret=%d)\n", prop, ret);

    return 0;
}
int skeleton_remove(struct platform_device* pdev)
{
    pr_info("skeleton - sysfs driver remove %llx\n", (unsigned long long)pdev);
    misc_deregister(&misc_device);
    return 0;
}
void skeleton_shutdown(struct platform_device* pdev)
{
    pr_info("skeleton - sysfs driver shutdown %s\n", pdev->name);
}

struct of_device_id of_skeleton[] = {
    {
        .compatible = "mydevice",
    },
    {},
};
MODULE_DEVICE_TABLE(of, of_skeleton);

static struct platform_driver sysfs_driver = {
    .probe    = skeleton_probe,
    .remove   = skeleton_remove,
    .shutdown = skeleton_shutdown,
    .driver =
        {
            .name           = "mydriver",
            .of_match_table = of_match_ptr(of_skeleton),
        },
};

static int __init skeleton_init(void)
{
    pr_info("Linux module skeleton loading...\n");
    int status = platform_driver_register(&sysfs_driver);
    pr_info("Linux module skeleton loaded\n");
    return status;
}

static void __exit skeleton_exit(void)
{
    pr_info("Linux module skeleton exiting...\n");
    platform_driver_unregister(&sysfs_driver);
    pr_info("Linux module skeleton unloaded\n");
}

module_init(skeleton_init);
module_exit(skeleton_exit);

MODULE_AUTHOR("Daniel Gachet <daniel.gachet@hefr.ch>");
MODULE_DESCRIPTION("Module skeleton");
MODULE_LICENSE("GPL");
