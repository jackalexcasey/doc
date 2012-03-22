/*
 * This work is licensed under the terms of the GNU GPL, version 2.
 */

#include <linux/module.h>

#define DRIVER_VERSION	"0.01"
#define DRIVER_AUTHOR	"Etienne Martineau <etmartin101@gmail.com>"
#define DRIVER_DESC	"template module"

#define DPRINTK(fmt, args...)	\
	do{	\
		if(debug) \
			printk(KERN_DEBUG "%s: " fmt, __func__ , ## args); \
} while (0)

static int debug=0;
MODULE_PARM_DESC(debug, "");
module_param(debug, bool, 0644);

static int __init init(void)
{
	pr_info(DRIVER_DESC " version: " DRIVER_VERSION);
	return 0;
}

static void __exit cleanup(void)
{
}

module_init(init);
module_exit(cleanup);

MODULE_VERSION(DRIVER_VERSION);
MODULE_LICENSE("GPL v2");
MODULE_AUTHOR(DRIVER_AUTHOR);
MODULE_DESCRIPTION(DRIVER_DESC);

