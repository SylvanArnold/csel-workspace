/* skeleton.c */
#include <linux/init.h>   /* needed for macros */
#include <linux/kernel.h> /* needed for debugging */
#include <linux/module.h> /* needed by all modules */

#include <linux/cdev.h>    /* needed for char device driver */
#include <linux/fs.h>      /* needed for device drivers */
#include <linux/uaccess.h> /* needed to copy data to/from user */

#include <linux/list.h>		/* needed for linked list processing */
#include <linux/device.h> /* needed for sysfs handling */
#include <linux/miscdevice.h>
#include <linux/of.h>
#include <linux/platform_device.h> /* needed for sysfs handling */

#define BUFFER_SZ 10000
struct mydata {
    char buffer[BUFFER_SZ];
    struct miscdevice misc;
    struct list_head node;
};

static int skeleton_open(struct inode* i, struct file* f)
{
    pr_info("skeleton : open operation... major:%d, minor:%d, p_data=%llx\n",
            imajor(i),
            iminor(i),
            (long long)f->private_data);
    return 0;
}

static int skeleton_release(struct inode* i, struct file* f)
{
    pr_info("skeleton: release operation...p_data=%llx\n",
            (long long)f->private_data);
    return 0;
}

static ssize_t skeleton_read(struct file* f,
                             char __user* buf,
                             size_t count,
                             loff_t* off)
{
    struct mydata* mydata = container_of(f->private_data, struct mydata, misc);

    // compute remaining bytes to copy, update count and pointers
    ssize_t remaining = strlen(mydata->buffer) - (ssize_t)(*off);
    char* ptr         = mydata->buffer + *off;
    if (count > remaining) count = remaining;
    *off += count;

    // copy required number of bytes
    if (copy_to_user(buf, ptr, count) != 0) count = -EFAULT;

    pr_info("skeleton: read operation...p_data=%llx read=%ld\n",
            (long long)f->private_data,
            count);

    return count;
}

static ssize_t skeleton_write(struct file* f,
                              const char __user* buf,
                              size_t count,
                              loff_t* off)
{
    struct mydata* mydata = container_of(f->private_data, struct mydata, misc);

    // compute remaining space in buffer and update pointers
    ssize_t remaining = sizeof(mydata->buffer) - (ssize_t)(*off);
    char* ptr         = mydata->buffer + *off;
    *off += count;

    // check if still remaining space to store additional bytes
    if (count >= remaining) count = -EIO;

    // store additional bytes into internal buffer
    if (count > 0) {
        ptr[count] = 0;  // make sure string is null terminated
        if (copy_from_user(ptr, buf, count)) count = -EFAULT;
    }

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

int drv_probe(struct platform_device* pdev)
{
    int ret                     = 0;
    struct device_node* dt_node = pdev->dev.of_node;
    struct mydata* mydata       = 0;
    const char* prop_str        = 0;

    pr_info("skeleton - driver probe %llx (%s)- M.m=%d.%d, dtnode=%p\n",
            (unsigned long long)pdev,
            pdev->name,
            MAJOR(pdev->dev.devt),
            MINOR(pdev->dev.devt),
            dt_node);

    ret = of_property_read_string(dt_node, "attribute", &prop_str);
    if (prop_str && ret == 0) pr_info("attribute=%s (ret=%d)\n", prop_str, ret);

    if (pdev->num_resources) {
        pr_info("resources: name=%s, start=%lld, end=%lld\n",
                pdev->resource[0].name,
                pdev->resource[0].start,
                pdev->resource[0].end);
    }

    if (dt_node) {
        int ret                      = 0;
        const char* prop_str         = 0;
        const unsigned int* prop_reg = 0;
        struct device_node* child    = 0;
        struct list_head* list       = devm_kzalloc(&pdev->dev, sizeof(struct list_head), GFP_KERNEL);
        if (list == 0) {
            dev_err(&pdev->dev,
                    "Failed to allocate resource for miscdev\n");
            return -ENOMEM;
        }
        INIT_LIST_HEAD(list);
        platform_set_drvdata(pdev, list);

        for_each_available_child_of_node(dt_node, child)
        {
            mydata = devm_kzalloc(&pdev->dev, sizeof(*mydata), GFP_KERNEL);
            pr_info("miscdev=%llx\n", (long long)mydata);
            if (mydata == 0) {
                dev_err(&pdev->dev,
                        "Failed to allocate resource for miscdev\n");
                return -ENOMEM;
            }
            list_add_tail (&(mydata->node), list);

            pr_info("child found: name=%s, fullname=%s\n",
                    child->name,
                    child->full_name);
            prop_reg = of_get_property(child, "reg", NULL);
            if (prop_reg != 0) {
                unsigned long reg = of_read_ulong(prop_reg, 1);
                pr_info("reg:%lu\n", reg);
            }

            ret = of_property_read_string(child, "attribute", &prop_str);
            if (prop_str && ret == 0)
                pr_info("attribute=%s (ret=%d)\n", prop_str, ret);

            /* register misc device ... */
            mydata->misc.minor = MISC_DYNAMIC_MINOR;
            mydata->misc.fops  = &fops;
            mydata->misc.name  = child->full_name;
            mydata->misc.mode  = 0664;
            ret                = misc_register(&(mydata->misc));
            if (ret != 0) {
                dev_err(&pdev->dev, "Failed to register miscdev\n");
                return ret;
            }
        }
    }

    return 0;
}
int drv_remove(struct platform_device* pdev)
{
	struct mydata* mydata;
    struct list_head* list = platform_get_drvdata(pdev);
    pr_info("skeleton - sysfs driver remove %s\n", pdev->name);
    list_for_each_entry (mydata, list, node) {
        misc_deregister(&(mydata->misc));
    }
    return 0;
}

void drv_shutdown(struct platform_device* pdev)
{
    pr_info("skeleton - sysfs driver shutdown %s\n", pdev->name);
}
int drv_suspend(struct platform_device* pdev, pm_message_t state)
{
    pr_info("skeleton - sysfs driver suspend %s (state=%d)\n",
            pdev->name,
            state.event);
    return 0;
}
int drv_resume(struct platform_device* pdev)
{
    pr_info("skeleton - sysfs driver resume %s\n", pdev->name);
    return 0;
}

struct of_device_id of_drv[] = {
    {
        .compatible = "mydevice",
    },
    {},
};
MODULE_DEVICE_TABLE(of, of_drv);

static struct platform_driver sysfs_driver = {
    .probe    = drv_probe,
    .remove   = drv_remove,
    .shutdown = drv_shutdown,
    .suspend  = drv_suspend,
    .resume   = drv_resume,
    .driver =
        {
            .name           = "mydriver",
            .of_match_table = of_match_ptr(of_drv),
        },
};
#if 1
static int __init sysfs_driver_init(void)
{
    int status = 0;

    pr_info("Linux module skeleton loading...\n");

    /* install sysfs */
    if (status == 0) status = platform_driver_register(&sysfs_driver);

    pr_info("Linux module skeleton loaded\n");
    return 0;
}

static void __exit sysfs_driver_exit(void)
{
    pr_info("Linux module skeleton exiting...\n");

    /* uninstall sysfs */
    platform_driver_unregister(&sysfs_driver);

    pr_info("Linux module skeleton unloaded\n");
}

module_init(sysfs_driver_init);
module_exit(sysfs_driver_exit);

#else
module_platform_driver(sysfs_driver);
#endif

MODULE_AUTHOR("Daniel Gachet <daniel.gachet@hefr.ch>");
MODULE_DESCRIPTION("Module skeleton");
MODULE_LICENSE("GPL");
