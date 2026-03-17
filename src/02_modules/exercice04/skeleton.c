// skeleton.c
#include <linux/module.h>	// needed by all modules
#include <linux/init.h>		// needed for macros
#include <linux/kernel.h>	// needed for debugging

#include <linux/moduleparam.h>  /* needed for module parameters */

#include <linux/slab.h> 
#include <linux/list.h>
#include <linux/string.h>

static char* text = "";
module_param(text, charp, 0);
static int  elements = 0;
module_param(elements, int, 0);

struct element {
	int id;
	char* text;
	struct list_head node;
};

static LIST_HEAD(elements_list);

// Create a list of elements with the specified text and id, and add them to the elements_list
static int __init skeleton_init(void)
{
	int i;
	pr_info ("Sylvan Module Loaded\n");
	if(elements > 0){
		pr_info ("Creating list of %d elements\n", elements);
		for (i = 0; i < elements; i++) {
			struct element* el = kmalloc(sizeof(*el), GFP_KERNEL); // Allocate memory for a new element
			if (el) {
				el->id = i;
				el->text = kmalloc(strlen(text) + 1, GFP_KERNEL); // Allocate memory for the text, including space for the null terminator, non-initialized
				if (el->text)
					strncpy(el->text, text, strlen(text) + 1); // Copy the text into the allocated memory
				else
					pr_warn("Failed to allocate text for element %d\n", i);
				list_add_tail(&el->node, &elements_list);
			} else {
				pr_warn("Failed to allocate element %d\n", i);
			}
		}
	}
	return 0;
}

// Free the memory allocated for each element in the elements_list and remove them from the list
static void __exit skeleton_exit(void)
{
	struct element* ele;
	struct element* tmp;

	list_for_each_entry_safe(ele, tmp, &elements_list, node) {
		pr_info("Deleting element %d", ele->id);
		list_del(&ele->node);
		kfree(ele->text);
		kfree(ele);
	}
	pr_info("Deleted %d elements\n", elements);
	pr_info("Sylvan Module Unloaded\n");
}

module_init (skeleton_init);
module_exit (skeleton_exit);

MODULE_AUTHOR ("Sylvan Arnold");
MODULE_DESCRIPTION ("Module to test dynamic memory allocation");
MODULE_LICENSE ("GPL");

