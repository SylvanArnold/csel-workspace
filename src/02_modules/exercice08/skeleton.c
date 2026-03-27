// skeleton.c

// Warning: a waitqueue is not like a message queue. It is used to put a thread to sleep until a condition is met.

#include <linux/module.h>	// needed by all modules
#include <linux/init.h>		// needed for macros
#include <linux/kernel.h>	// needed for debugging

#include <linux/gpio.h> 
#include <linux/interrupt.h>	

// Define the GPIO pins to use for the buttons
#define K1 0
#define K2 2
#define K3 3

enum Switch {K1_Switch, K2_Switch, K3_Switch};

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

	return IRQ_HANDLED;
}


static int __init skeleton_init(void)
{

	if (gpio_request(K1, "k1") == 0) {
		if (request_irq(gpio_to_irq(K1), gpio_isr, IRQF_TRIGGER_FALLING | IRQF_SHARED, "k1", (void*)K1_Switch) != 0) {
			pr_err("Failed to request IRQ for GPIO %d\n", K1);
			gpio_free(K1);
		}
	} else {
		pr_err("Failed to request GPIO %d for k1\n", K1);
	}

	if (gpio_request(K2, "k2") == 0) {
	if (request_irq(gpio_to_irq(K2), gpio_isr, IRQF_TRIGGER_FALLING | IRQF_SHARED, "k2", (void*)K2_Switch) != 0) {
		pr_err("Failed to request IRQ for GPIO %d\n", K2);
		gpio_free(K2);
	}
	} else {
		pr_err("Failed to request GPIO %d for k2\n", K2);
	}

	if (gpio_request(K3, "k3") == 0) {
	if (request_irq(gpio_to_irq(K3), gpio_isr, IRQF_TRIGGER_FALLING | IRQF_SHARED, "k3", (void*)K3_Switch) != 0) {
		pr_err("Failed to request IRQ for GPIO %d\n", K3);
		gpio_free(K3);
	}
	} else {
		pr_err("Failed to request GPIO %d for k3\n", K3);
	}

	pr_info ("Exercice 8 module loaded\n");
	return 0;
}

static void __exit skeleton_exit(void)
{
	gpio_free(K1);
	free_irq(gpio_to_irq(K1), (void*)K1_Switch);
	gpio_free(K2);
	free_irq(gpio_to_irq(K2), (void*)K2_Switch);
	gpio_free(K3);
	free_irq(gpio_to_irq(K3), (void*)K3_Switch);
	pr_info("Exercice 8 Module Unloaded\n");
}

module_init (skeleton_init);
module_exit (skeleton_exit);

MODULE_AUTHOR ("Sylvan Arnold");
MODULE_DESCRIPTION ("Module that handle interrupts from gpio pins");
MODULE_LICENSE ("GPL");

