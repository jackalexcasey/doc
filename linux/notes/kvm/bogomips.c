/*
 * bogomips.c - Spinloop calibration
 *
 * Copyright (C) 2010 Cisco Systems
 * Author: Etienne Martineau <etmartin@cisco.com>
 *
 * This work is licensed under the terms of the GNU GPL, version 2.
 *
 */

#include <linux/module.h>
#include <linux/sched.h>
#include <linux/timex.h>
#include <linux/jiffies.h>
#include <linux/timex.h>
#include <linux/preempt.h>
#include <linux/delay.h>
#include <linux/init.h>
#include <linux/stop_machine.h>
#include <linux/io.h>

#include <asm/processor.h>
#include <asm/delay.h>
#include <asm/timer.h>
#include <asm/tlbflush.h>

#ifdef CONFIG_SMP
# include <asm/smp.h>
#endif

#define DRIVER_VERSION	"0.01"
#define DRIVER_AUTHOR	"Etienne Martineau <etmartin@cisco.com>"
#define DRIVER_DESC	"Spinloop calibration"
#define DPRINTK(fmt, args...)	\
	do{	\
		if(debug) \
			printk(KERN_DEBUG "%s: " fmt, __func__ , ## args); \
} while (0)

static int debug=0;
MODULE_PARM_DESC(debug, "");
module_param(debug, bool, 0644);

static long capture=0;
MODULE_PARM_DESC(capture, "");
module_param(capture, long, 0644);

static int __init init(void)
{
	unsigned long *ptr;
	printk("Accessing MEMORY %Lx\n",capture);
	flush_tlb_all();

	ptr = ioremap(capture,4096);
	if(!ptr)
		printk("Cannot ioremap\n");
	else{
		printk("GVA = %p; GPA = %Lx\n",ptr,capture);
		ptr[0] = 1234;
		iounmap(ptr);
	}

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

