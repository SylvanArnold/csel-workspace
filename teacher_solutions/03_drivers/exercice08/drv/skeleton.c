/* skeleton.c */
#include <linux/init.h>   /* needed for macros */
#include <linux/kernel.h> /* needed for debugging */
#include <linux/module.h> /* needed by all modules */

#include <linux/cdev.h>    /* needed for char device driver */
#include <linux/fs.h>      /* needed for device drivers */
#include <linux/uaccess.h> /* needed to copy data to/from user */

#include <linux/mm.h> /* needed for mmap handling */

#include <linux/device.h>          /* needed for sysfs handling */
#include <linux/platform_device.h> /* needed for sysfs handling */

static int skeleton_mmap(struct file* f, struct vm_area_struct* vma)
{
    int status         = 0;
    unsigned long size = vma->vm_end - vma->vm_start;

    if (size > PAGE_SIZE) status = -EINVAL;

    vma->vm_pgoff = 0x01c14000 >> PAGE_SHIFT;

    if (status == 0)
        status = remap_pfn_range(
            vma, vma->vm_start, vma->vm_pgoff, size, vma->vm_page_prot);
    if (status != 0) status = -EAGAIN;

    pr_info("skeleton: mmap (size=%lu, shift=%d) status=%d\n",
            size,
            PAGE_SHIFT,
            status);
    return status;
}

static struct file_operations skeleton_fops = {
    .owner = THIS_MODULE,
    .mmap  = skeleton_mmap,
};

static dev_t skeleton_dev;
static struct cdev skeleton_cdev;

static void sysfs_dev_release(struct device* dev)
{
    pr_info("skeleton - sysfs dev release\n");
}
static struct platform_device sysfs_device;

static int __init skeleton_init(void)
{
    int status;

    status = alloc_chrdev_region(&skeleton_dev, 0, 1, "mymodule");
    if (status == 0) {
        cdev_init(&skeleton_cdev, &skeleton_fops);
        skeleton_cdev.owner = THIS_MODULE;
        status              = cdev_add(&skeleton_cdev, skeleton_dev, 1);
    }

    sysfs_device.name        = "mymodule";
    sysfs_device.id          = -1;
    sysfs_device.dev.release = sysfs_dev_release;
    sysfs_device.dev.devt    = skeleton_dev;
    if (status == 0) status = platform_device_register(&sysfs_device);

    pr_info("Linux module skeleton loaded\n");
    return 0;
}

static void __exit skeleton_exit(void)
{
    platform_device_unregister(&sysfs_device);
    cdev_del(&skeleton_cdev);
    unregister_chrdev_region(skeleton_dev, 1);

    pr_info("Linux module skeleton unloaded\n");
}

module_init(skeleton_init);
module_exit(skeleton_exit);

MODULE_AUTHOR("Daniel Gachet <daniel.gachet@hefr.ch>");
MODULE_DESCRIPTION("Module skeleton");
MODULE_LICENSE("GPL");
