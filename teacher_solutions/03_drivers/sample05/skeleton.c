/* skeleton.c */
#include <linux/init.h>   /* needed for macros */
#include <linux/kernel.h> /* needed for debugging */
#include <linux/module.h> /* needed by all modules */
#include <linux/device.h> /* needed for sysfs handling */

static char sysfs_buf[1000];

ssize_t sysfs_show_attr(struct device* dev,
                        struct device_attribute* attr,
                        char* buf)
{
    strcpy(buf, sysfs_buf);
    return strlen(buf);
}
ssize_t sysfs_store_attr(struct device* dev,
                         struct device_attribute* attr,
                         const char* buf,
                         size_t count)
{
    int len = sizeof(sysfs_buf) - 1;
    if (len > count) len = count;
    strncpy(sysfs_buf, buf, len);
    sysfs_buf[len] = 0;
    return len;
}
DEVICE_ATTR(attr, 0664, sysfs_show_attr, sysfs_store_attr);

static struct class* sysfs_class;
static struct device* sysfs_device;

static int __init skeleton_init(void)
{
    int status = 0;

    sysfs_class = class_create(THIS_MODULE, "myclass");
    sysfs_device = device_create(sysfs_class, NULL, 0, NULL, "mydevice");
    status = device_create_file(sysfs_device, &dev_attr_attr);

    pr_info("Linux module skeleton loaded\n");
    return status;
}

static void __exit skeleton_exit(void)
{
    /* uninstall sysfs */
    device_remove_file(sysfs_device, &dev_attr_attr);
    device_destroy(sysfs_class, 0);
    class_destroy(sysfs_class);

    pr_info("Linux module skeleton unloaded\n");
}

module_init(skeleton_init);
module_exit(skeleton_exit);

MODULE_AUTHOR("Daniel Gachet <daniel.gachet@hefr.ch>");
MODULE_DESCRIPTION("Module skeleton");
MODULE_LICENSE("GPL");
