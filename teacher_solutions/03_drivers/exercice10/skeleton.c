/* skeleton.c */
#include <linux/init.h>   /* needed for macros */
#include <linux/kernel.h> /* needed for debugging */
#include <linux/module.h> /* needed by all modules */

#include <linux/cdev.h>    /* needed for char device driver */
#include <linux/fs.h>      /* needed for device drivers */
#include <linux/uaccess.h> /* needed to copy data to/from user */

#include <linux/proc_fs.h> /* needed for procfs handling */

struct skeleton_config {
    int id;
    long ref;
    char name[50];
    char descr[50];
};

static struct skeleton_config config = {.id    = 11,
                                        .ref   = 33,
                                        .name  = "config structure",
                                        .descr = "procfs test driver"};
static int val                       = 55;

static ssize_t skeleton_read_config(struct file* f,
                                    char __user* buf,
                                    size_t count,
                                    loff_t* off)
{
    char temp[200];
    int len = snprintf(temp,
                       sizeof(temp),
                       "id=%d\nref=%ld\nname=%s\ndescr=%s\n",
                       config.id,
                       config.ref,
                       config.name,
                       config.descr);

    len -= (ssize_t)(*off);
    if (count > len) count = len;
    *off += count;

    if (copy_to_user(buf, temp, count) != 0) count = -EFAULT;
    return count;
}

static const struct proc_ops fops_config = {
    .proc_read = skeleton_read_config,
};

static ssize_t skeleton_read_val(struct file* f,
                                 char __user* buf,
                                 size_t count,
                                 loff_t* off)
{
    char temp[20];
    int len = snprintf(temp, sizeof(temp), "%d\n", val);

    len -= (ssize_t)(*off);
    if (count > len) count = len;
    *off += count;

    if (copy_to_user(buf, temp, count) != 0) count = -EFAULT;
    return count;
}

static ssize_t skeleton_write_val(struct file* f,
                                  const char __user* buf,
                                  size_t count,
                                  loff_t* off)
{
    char temp[20];

    if (count > sizeof(temp)) return -EIO;
    if (copy_from_user(temp, buf, count)) return -EFAULT;

    val = simple_strtol(temp, 0, 10);

    return count;
}

static const struct proc_ops fops_val = {
    .proc_read  = skeleton_read_val,
    .proc_write = skeleton_write_val,
};

static struct proc_dir_entry* procfs_dir = 0;

static int __init skeleton_init(void)
{
    int status = 0;

    /* create procfs node and attributes */
    procfs_dir = proc_mkdir("mymodule", NULL);
    proc_create("config", 0, procfs_dir, &fops_config);
    proc_create("val", 0, procfs_dir, &fops_val);

    if (procfs_dir == 0) status = -EFAULT;

    pr_info("Linux module skeleton loaded (status=%d)\n", status);

    return status;
}

static void __exit skeleton_exit(void)
{
    /* remove procfs attributes and node */
    remove_proc_entry("val", procfs_dir);
    remove_proc_entry("config", procfs_dir);
    remove_proc_entry("mymodule", NULL);

    pr_info("Linux module skeleton unloaded\n");
}

module_init(skeleton_init);
module_exit(skeleton_exit);

MODULE_AUTHOR("Daniel Gachet <daniel.gachet@hefr.ch>");
MODULE_DESCRIPTION("Module skeleton");
MODULE_LICENSE("GPL");
