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

#define BUFFER_SZ 10000

static struct mydata {
    struct device* device;

    struct skeleton_attr {
        char buffer[BUFFER_SZ];
        struct device_attribute dev_attr;
    } attr;

} mydevices[5];

#define ATTR_INSTANCE(a)                              \
    {                                                 \
        a.attr.buffer[0]          = 0;                \
        a.attr.dev_attr.attr.name = "val";            \
        a.attr.dev_attr.attr.mode = 0664;             \
        a.attr.dev_attr.show      = sysfs_show_attr;  \
        a.attr.dev_attr.store     = sysfs_store_attr; \
    }

// --- sysfs file opers ------------------------------------------------------

ssize_t sysfs_show_attr(struct device* dev,
                        struct device_attribute* attr,
                        char* buf)
{
    struct mydata* mydata = container_of(attr, struct mydata, attr.dev_attr);

    strcpy(buf, mydata->attr.buffer);
    return strlen(buf);
}

ssize_t sysfs_store_attr(struct device* dev,
                         struct device_attribute* attr,
                         const char* buf,
                         size_t count)
{
    struct mydata* mydata = container_of(attr, struct mydata, attr.dev_attr);

    int len = sizeof(mydata->attr.buffer) - 1;
    if (len > count) len = count;
    strncpy(mydata->attr.buffer, buf, len);
    mydata->attr.buffer[len] = 0;
    return len;
}
static struct class* sysfs_class;

static int __init skeleton_init(void)
{
    int status = 0;
    int i =0;

    sysfs_class = class_create(THIS_MODULE, "myclass");

    for (i=0; i<ARRAY_SIZE(mydevices); i++) {
        struct mydata* mydev = &mydevices[i];

        mydev->device = device_create(sysfs_class, NULL, i, NULL, "mydevice.%d", i);

        ATTR_INSTANCE((*mydev));
        status = device_create_file(mydev->device, &(mydev->attr.dev_attr));
    }

    pr_info("Linux module skeleton loaded\n");
    return status;
}

static void __exit skeleton_exit(void)
{
    int i = 0;
    for (i=0; i<ARRAY_SIZE(mydevices); i++) {
        struct mydata* mydev = &mydevices[i];
        device_remove_file(mydev->device, &(mydev->attr.dev_attr));
        device_destroy(sysfs_class, i);
    }
    class_destroy(sysfs_class);

    pr_info("Linux module skeleton unloaded\n");
}

module_init(skeleton_init);
module_exit(skeleton_exit);

MODULE_AUTHOR("Daniel Gachet <daniel.gachet@hefr.ch>");
MODULE_DESCRIPTION("Module skeleton");
MODULE_LICENSE("GPL");
