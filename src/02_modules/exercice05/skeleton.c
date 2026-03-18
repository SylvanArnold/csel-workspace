// skeleton.c
#include <linux/module.h>	// needed by all modules
#include <linux/init.h>		// needed for macros
#include <linux/kernel.h>	// needed for debugging

#include <linux/ioport.h>	
#include <linux/io.h>

//static struct resource *resources[3];

static int __init skeleton_init(void)
{
	void __iomem *registers[3] = {0,0,0};
	unsigned int chipID[4] = {0,0,0,0};
	long cpu_temp = 0;
	unsigned int macAddr[2] = {0,0};

	pr_info ("Exercice 5 module loaded, reading registers...\n");

	// Request memory regions
	/*
	resources[0] = request_mem_region (0x01c14200, 0x10, "Chip ID");
	resources[1] = request_mem_region (0x01c25080, 0x4, "CPU Temperature");
	resources[2] = request_mem_region (0x01c30050, 0x8, "MAC Address");

	if ((ressouces[0] == 0) || (ressouces[1] == 0) ||(ressouces[2] == 0)) {
		pr_info ("Failed to reserve memory regions");
		return -EFAULT;
	}
	*/
	
	// map register in virtual memory
	registers[0] = ioremap (0x01c14200, 0x10);
	registers[1] = ioremap (0x01c25080, 0x4);
	registers[2] = ioremap (0x01c30050, 0x8);

	if ((registers[0] == 0) || (registers[1] == 0) ||(registers[2] == 0)) {
		pr_info ("Failded to map processors registers\n");
		if (registers[0]) iounmap(registers[0]);
		if (registers[1]) iounmap(registers[1]);
		if (registers[2]) iounmap(registers[2]);
		return -EFAULT;
	}



	chipID[0] = ioread32 (registers[0]);
	chipID[1] = ioread32 (registers[0]+0x4);
	chipID[2] = ioread32 (registers[0]+0x8);
	chipID[3] = ioread32 (registers[0]+0xc);
	pr_info("Chip ID: %08x'%08x'%08x'%08x\n", chipID[0], chipID[1], chipID[2], chipID[3]);

	cpu_temp = ioread32 (registers[1]);
	cpu_temp = -1191*cpu_temp/10+223000;
	pr_info("CPU Temperature: %ld m°C\n", cpu_temp);

	macAddr[0] = ioread32 (registers[2]);
	macAddr[1] = ioread32 (registers[2]+0x4);
	
	pr_info("MAC Address: %02x:%02x:%02x:%02x:%02x:%02x\n", (macAddr[1]>>0)&0xff, (macAddr[1]>>8)&0xff, (macAddr[1]>>16)&0xff, (macAddr[1]>>24)&0xff, (macAddr[0]>>0)&0xff, (macAddr[0]>>8)&0xff);	

	iounmap(registers[0]);
	iounmap(registers[1]);
	iounmap(registers[2]);

	return 0;
}

static void __exit skeleton_exit(void)
{
	/*
	release_mem_region (0x01c14200, 0x10);
	release_mem_region (0x01c25080, 0x4);
	release_mem_region (0x01c30050, 0x8);
	*/

	pr_info("Exercice 5 Module Unloaded\n");
}

module_init (skeleton_init);
module_exit (skeleton_exit);

MODULE_AUTHOR ("Sylvan Arnold");
MODULE_DESCRIPTION ("Module that displays chip id, cpu temperature and MAC address");
MODULE_LICENSE ("GPL");

