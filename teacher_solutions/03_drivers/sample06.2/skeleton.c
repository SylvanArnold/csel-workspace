/* skeleton.c */
#include <linux/module.h>	/* needed by all modules */
#include <linux/init.h>		/* needed for macros */
#include <linux/kernel.h>	/* needed for debugging */

#include <linux/moduleparam.h>	/* needed for module parameters */
#include <linux/slab.h>		/* needed for dynamic memory management */

#include <linux/fs.h>		/* needed for device drivers */
#include <linux/uaccess.h>	/* needed to copy data to/from user */
#include <linux/cdev.h>		/* needed for char device driver */

#include <linux/device.h>		/* needed for sysfs handling */
#include <linux/platform_device.h>	/* needed for sysfs handling */

#define INSTANCES 3

// --- sysfs operations --------------------------------------------------------

struct skeleton_attr {
	long val;
	int id;
	struct device_attribute dev_attr;
};

#define ATTR_INSTANCE(i,v)\
	{\
		.val = v,\
		.id = i,\
		.dev_attr = {\
			.attr  = {.name = "val", .mode=0664, },\
			.show  = sysfs_show_val,\
			.store = sysfs_store_val,\
		},\
	}

static ssize_t sysfs_show_val(
	struct device *dev,
	struct device_attribute *attr,
	char *buf)
{
	struct skeleton_attr* s_attr =
		container_of(attr, struct skeleton_attr, dev_attr);

	sprintf (buf, "%ld\n", s_attr->val);

	pr_info ("skeleton-sysfs_show: val[%d]=%ld\n", s_attr->id, s_attr->val);

	return strlen(buf);
}

static ssize_t sysfs_store_val(
	struct device *dev,
	struct device_attribute *attr,
	const char *buf, size_t count)
{
	struct skeleton_attr* s_attr =
		container_of(attr, struct skeleton_attr, dev_attr);

	int err = kstrtol (buf, 10, &s_attr->val);

	//pr_info ("skeleton-sysfs_store: val[%d]=%ld\n", s_attr->id, s_attr->val);
	return err == 0 ? count : err;
}

static struct skeleton_attr skeleton_attr[INSTANCES] = {
	[0] = ATTR_INSTANCE(0,10),
	[1] = ATTR_INSTANCE(1,11),
	[2] = ATTR_INSTANCE(2,12),
};


static void sysfs_dev_release (struct device *dev)
{
    pr_info ("skeleton-sysfs_dev_release\n");
}
static struct platform_device sysfs_device[INSTANCES];

// --- fops operations ---------------------------------------------------------

#define BUFFER_SZ 10000
static char buffers[INSTANCES][BUFFER_SZ];

static int skeleton_open (struct inode *i, struct file *f)
{
	pr_info ("skeleton-open operation... major:%d, minor:%d\n",
		imajor (i), iminor(i));

	if ((f->f_mode & (FMODE_READ | FMODE_WRITE)) != 0) {
		pr_info (" o--> opened for reading & writing...\n");
	} else if (iminor(i) >= INSTANCES) return -EFAULT;

	if ((f->f_mode & FMODE_READ) != 0) {
		pr_info (" o--> opened for reading...\n");

	} else if ((f->f_mode & FMODE_WRITE) != 0) {
		pr_info (" o--> opened for writing...\n");
	}

	f->private_data = buffers[iminor(i)];
	pr_info (" o--> private_data address = %p\n", f->private_data);

	return 0;
}

static int skeleton_release (struct inode *i, struct file *f)
{
	pr_info ("skeleton-release operation...\n");

	return 0;
}

static ssize_t skeleton_read (struct file *f, char __user *buf,
	size_t count, loff_t *off)
{
	// compute remaining bytes to copy, update count and pointers
	ssize_t remaining = strlen(f->private_data) - (ssize_t)(*off);
	char* ptr = (char*)f->private_data + *off;
	if (count > remaining) count = remaining;
	*off += count;

	// copy required number of bytes
	if (copy_to_user (buf, ptr, count) != 0) count = -EFAULT;

	pr_info("skeleton-read: buf=%p, read=%ld\n", f->private_data, count);

	return count;
}

static ssize_t skeleton_write (struct file *f, const char __user *buf,
	size_t count, loff_t *off)
{
	// compute remaining space in buffer and update pointers
	ssize_t remaining = BUFFER_SZ - (ssize_t)(*off);
	char* ptr = (char*)f->private_data + *off;
	*off += count;

	// check if still remaining space to store additional bytes
	if (count >= remaining) count = -EIO;

	// store additional bytes into internal buffer
	if (count > 0) {
		ptr[count] = 0;  // make sure string is null terminated
		if (copy_from_user (ptr, buf, count)) count = -EFAULT;
	}

	pr_info ("skeleton-write: buf=%p, written=%ld\n", f->private_data, count);

	return count;
}

static struct file_operations skeleton_fops = {
	.owner = THIS_MODULE,
	.open = skeleton_open,
	.read = skeleton_read,
	.write = skeleton_write,
	.release = skeleton_release,
};

static dev_t skeleton_dev;
static struct cdev skeleton_cdev;

static int __init skeleton_init(void)
{
	int i;
	int status = -EFAULT;

	status = alloc_chrdev_region (&skeleton_dev, 0, INSTANCES, "mymodule");
	if (status == 0) {
		cdev_init (&skeleton_cdev, &skeleton_fops);
		skeleton_cdev.owner = THIS_MODULE;
		status = cdev_add (&skeleton_cdev, skeleton_dev, INSTANCES);
	}

	/* install sysfs */
	for (i = 0; i < INSTANCES; i++) {
		sysfs_device[i].name = "mymodule";
		sysfs_device[i].id = i;
		sysfs_device[i].dev.release = sysfs_dev_release;
		sysfs_device[i].dev.devt = MKDEV (MAJOR(skeleton_dev), i);
		if (status == 0)
			status = platform_device_register (&sysfs_device[i]);
 		if (status == 0)
			status = device_create_file (
				&sysfs_device[i].dev,
				&skeleton_attr[i].dev_attr);
	}

	pr_info ("Linux module skeleton loaded\n");

	return status;
}

static void __exit skeleton_exit(void)
{
	int i;

	for (i = 0; i < INSTANCES; i++) {
		device_remove_file (
			&sysfs_device[i].dev,
			&skeleton_attr[i].dev_attr);
		platform_device_unregister (&sysfs_device[i]);
	}

	cdev_del (&skeleton_cdev);
	unregister_chrdev_region (skeleton_dev, INSTANCES);

	pr_info ("Linux module skeleton unloaded\n");
}

module_init (skeleton_init);
module_exit (skeleton_exit);

MODULE_AUTHOR ("Daniel Gachet <daniel.gachet@hefr.ch>");
MODULE_DESCRIPTION ("Module skeleton");
MODULE_LICENSE ("GPL");

