// skeleton.c
#include <linux/module.h>	// needed by all modules
#include <linux/init.h>		// needed for macros
#include <linux/kernel.h>	// needed for debugging

#include <linux/kthread.h>	
#include <linux/delay.h>

static struct task_struct* ex6thread;

static int thread_func (void* data)
{
	pr_info ("Thread is running\n");
	while(!kthread_should_stop()) {
		ssleep (5);
		pr_info ("tick\n");
	}
	return 0;
}

static int __init skeleton_init(void)
{
	pr_info ("Exercice 6 module loaded, reading registers...\n");

	ex6thread = kthread_run (thread_func, NULL, "ex6thread");
	if (IS_ERR(ex6thread)) {
		pr_info ("Failed to create thread\n");
		return -EFAULT;
	}

	return 0;
}

static void __exit skeleton_exit(void)
{
	kthread_stop (ex6thread);
	pr_info("Exercice 6 Module Unloaded, thread stopped\n");
}

module_init (skeleton_init);
module_exit (skeleton_exit);

MODULE_AUTHOR ("Sylvan Arnold");
MODULE_DESCRIPTION ("Module that creates a thread that prints a message every 5 seconds");
MODULE_LICENSE ("GPL");

