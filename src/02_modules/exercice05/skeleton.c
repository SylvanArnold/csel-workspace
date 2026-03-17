// skeleton.c
#include <linux/module.h>	// needed by all modules
#include <linux/init.h>		// needed for macros
#include <linux/kernel.h>	// needed for debugging

static int __init skeleton_init(void)
{
	pr_info ("Exercice 5 module loaded, reading registers...\n");
	return 0;
}

static void __exit skeleton_exit(void)
{
	pr_info("Exercice 5 Module Unloaded\n");
}

module_init (skeleton_init);
module_exit (skeleton_exit);

MODULE_AUTHOR ("Sylvan Arnold");
MODULE_DESCRIPTION ("Module that displays chip id, cpu temperature and MAC address");
MODULE_LICENSE ("GPL");

