// skeleton.c

// Warning: a waitqueue is not like a message queue. It is used to put a thread to sleep until a condition is met.

#include <linux/module.h>	// needed by all modules
#include <linux/init.h>		// needed for macros
#include <linux/kernel.h>	// needed for debugging

#include <linux/kthread.h>	
#include <linux/delay.h>
#include <linux/wait.h>

static struct task_struct* ex7thread_1;
static struct task_struct* ex7thread_2;

DECLARE_WAIT_QUEUE_HEAD(queue);
static atomic_t is_kicked;

static int thread_func_1 (void* data)
{
	pr_info ("Thread 1 is running\n");
	while(!kthread_should_stop()) {
		ssleep (5);
		pr_info ("Thread 1: notifying thread 2\n");
		atomic_set (&is_kicked, 1); // set the kicked condition to true
		wake_up_interruptible (&queue); // wake up thread 2 if it is sleeping on the queue
	}
	return 0;
}

static int thread_func_2 (void* data)
{
	pr_info ("Thread 2 is running\n");
	while(!kthread_should_stop()) {
		// TODO: wait for thread 1 notification
		wait_event_interruptible (queue, atomic_read (&is_kicked) || kthread_should_stop());
		if(atomic_read (&is_kicked)) {
			pr_info ("Thread 2: received notification from thread 1\n");
			atomic_set (&is_kicked, 0); // reset the kicked condition
		}
	}
	return 0;
}


static int __init skeleton_init(void)
{
	pr_info ("Exercice 7 module loaded, reading registers...\n");

	init_waitqueue_head(&queue);
	atomic_set (&is_kicked, 0);

	ex7thread_1 = kthread_run (thread_func_1, NULL, "ex7thread_1");
	if (IS_ERR(ex7thread_1)) {
		pr_info ("Failed to create thread 1\n");
		return -EFAULT;
	}

	ex7thread_2 = kthread_run (thread_func_2, NULL, "ex7thread_2");
	if (IS_ERR(ex7thread_2)) {
		pr_info ("Failed to create thread 2\n");
		return -EFAULT;
	}

	return 0;
}

static void __exit skeleton_exit(void)
{
	kthread_stop (ex7thread_1);
	kthread_stop (ex7thread_2);
	pr_info("Exercice 7 Module Unloaded, threads stopped\n");
}

module_init (skeleton_init);
module_exit (skeleton_exit);

MODULE_AUTHOR ("Sylvan Arnold");
MODULE_DESCRIPTION ("Module that creates two threads that communicates with each other");
MODULE_LICENSE ("GPL");

