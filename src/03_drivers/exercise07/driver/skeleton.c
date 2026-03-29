// skeleton.c

// Warning: a waitqueue is not like a message queue. It is used to put a thread to sleep until a condition is met.

#include <linux/module.h>	// needed by all modules
#include <linux/init.h>		// needed for macros
#include <linux/kernel.h>	// needed for debugging

#include <linux/gpio.h> 
#include <linux/interrupt.h>	
#include <linux/cdev.h> 
#include <linux/fs.h>  
#include <linux/poll.h> 
#include <linux/uaccess.h>

#include <linux/miscdevice.h>

// Define the GPIO pins to use for the buttons
#define K1 0
#define K2 2
#define K3 3

enum Switch {K1_Switch, K2_Switch, K3_Switch};

DECLARE_WAIT_QUEUE_HEAD(queue);

static atomic_t nb_of_interrupts = ATOMIC_INIT(0);

irqreturn_t gpio_isr(int irq, void* handle)
{
	enum Switch switch_pressed = (enum Switch)handle; // Cast the handle to the Switch enum

	switch (switch_pressed) {
		case K1_Switch:
			pr_info("Interrupt from K1\n");
			break;
		case K2_Switch:
			pr_info("Interrupt from K2\n");
			break;
		case K3_Switch:
			pr_info("Interrupt from K3\n");
			break;
		default:
			pr_err("Unknown switch interrupt\n");
			break;
	}

	wake_up_interruptible(&queue);
	atomic_inc(&nb_of_interrupts);
	
	return IRQ_HANDLED;
}

// tell user space that data is available to read
static unsigned int skeleton_poll(struct file* f, poll_table* wait)
{
    unsigned mask = 0;
    poll_wait(f, &queue, wait);

	if (atomic_read(&nb_of_interrupts) != 0) { // Check if there are pending interrupts
		mask |= POLLIN | POLLRDNORM; // Tell user space that data is available to read
	}
	pr_info("polling thread waked-up...\n");
    
    return mask;
}

// read function to return the number of interrupts that have occurred since the last read
// caller can choose between blocking and non-blocking mode by setting the O_NONBLOCK flag when opening the device file
static ssize_t skeleton_read(struct file* f, char __user* buf,size_t sz,loff_t* off){
    // return the number of interrupts that have occurred since the last read
	int interrupts = atomic_xchg(&nb_of_interrupts, 0);

	if (interrupts == 0) {
		if (f->f_flags & O_NONBLOCK) {
			return -EAGAIN;
		}
		// if we are in blocking mode, we should wait for an interrupt
		if (wait_event_interruptible(queue, atomic_read(&nb_of_interrupts) != 0)) {
			return -ERESTARTSYS;
		}
		interrupts = atomic_xchg(&nb_of_interrupts, 0);
	}

	char buffer[16]; // tmp buffer to hold the string representation of the interrupt count

	// convert the interrupt count to a string
	int len = snprintf(buffer, sizeof(buffer), "%d\n", interrupts); // Convert the interrupt
	if (copy_to_user(buf, buffer, len)) {
		return -EFAULT;
	}
	return len;
}

static struct file_operations skeleton_fops = {
    .owner = THIS_MODULE,
    .read  = skeleton_read,
    .poll  = skeleton_poll,
};

struct miscdevice misc_device = {
    .minor = MISC_DYNAMIC_MINOR,
    .fops  = &skeleton_fops,
    .name  = "switch_driver",
    .mode  = 0777,
};

static int __init skeleton_init(void)
{
	int status;

	status = misc_register(&misc_device); // Register the misc device
	if (status != 0) {
		pr_err("Failed to register misc device\n");
		return status;
	}

	if (gpio_request(K1, "k1") == 0) {
		gpio_direction_input(K1);
		if (request_irq(gpio_to_irq(K1), gpio_isr, IRQF_TRIGGER_FALLING, "k1", (void*)K1_Switch) != 0) {
			pr_err("Failed to request IRQ for GPIO %d\n", K1);
			gpio_free(K1);
		}
	} else {
		pr_err("Failed to request GPIO %d for k1\n", K1);
	}

	if (gpio_request(K2, "k2") == 0) {
		gpio_direction_input(K2);
		if (request_irq(gpio_to_irq(K2), gpio_isr, IRQF_TRIGGER_FALLING, "k2", (void*)K2_Switch) != 0) {
			pr_err("Failed to request IRQ for GPIO %d\n", K2);
			gpio_free(K2);
		}
	} else {
		pr_err("Failed to request GPIO %d for k2\n", K2);
	}

	if (gpio_request(K3, "k3") == 0) {
		gpio_direction_input(K3);
		if (request_irq(gpio_to_irq(K3), gpio_isr, IRQF_TRIGGER_FALLING, "k3", (void*)K3_Switch) != 0) {
			pr_err("Failed to request IRQ for GPIO %d\n", K3);
			gpio_free(K3);
		}
	} else {
		pr_err("Failed to request GPIO %d for k3\n", K3);
	}

	pr_info ("Exercice 7 module loaded\n");
	return 0;
}

static void __exit skeleton_exit(void)
{
	misc_deregister(&misc_device);

	free_irq(gpio_to_irq(K1), (void*)K1_Switch);
	gpio_free(K1);
	free_irq(gpio_to_irq(K2), (void*)K2_Switch);
	gpio_free(K2);
	free_irq(gpio_to_irq(K3), (void*)K3_Switch);
	gpio_free(K3);
	pr_info("Exercice 7 Module Unloaded\n");
}

module_init (skeleton_init);
module_exit (skeleton_exit);

MODULE_AUTHOR ("Sylvan Arnold");
MODULE_DESCRIPTION ("Driver for buttons with interrupts");
MODULE_LICENSE ("GPL");

