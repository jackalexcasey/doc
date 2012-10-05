/*
 * bogomips.c - Spinloop calibration
 *
 * Copyright (C) 2010 Cisco Systems
 * Author: Etienne Martineau <etmartin@cisco.com>
 *
 * This work is licensed under the terms of the GNU GPL, version 2.
 *
 */

#ifdef __KERNEL__
#include <linux/module.h>
#include <linux/sched.h>
#include <linux/timex.h>
#include <linux/jiffies.h>
#include <linux/timex.h>
#include <linux/preempt.h>
#include <linux/delay.h>
#include <linux/init.h>
#include <linux/miscdevice.h>
#include <linux/stop_machine.h>
#include <linux/sched.h>
#include <linux/errno.h>
#include <linux/miscdevice.h>
#include <linux/fcntl.h>

#include <linux/rtc.h>
#include <linux/init.h>
#include <linux/poll.h>
#include <linux/proc_fs.h>
#include <linux/mutex.h>

#include <asm/processor.h>
#include <asm/delay.h>
#include <asm/timer.h>

#ifdef CONFIG_SMP
# include <asm/smp.h>
#endif

#define DRIVER_VERSION	"0.01"
#define DRIVER_AUTHOR	"Etienne Martineau <etmartin@cisco.com>"
#define DRIVER_DESC	"Spinloop calibration"
#define PRINT(fmt, args...)	\
	do{	\
		printk(KERN_DEBUG fmt, ## args); \
} while (0)

#define MIN(a,b) (((a)<(b))?(a):(b)) 
#define MAX(a,b) (((a)>(b))?(a):(b))

#define MAX_LOOPS_NR 1024
#define MAX_CPU_NR 64

static int l=0;
MODULE_PARM_DESC(l, "loop number");
module_param(l, int, 0644);

static int j=0;
MODULE_PARM_DESC(j, "lpj");
module_param(j, int, 0644);

static DEFINE_SPINLOCK(lock);

#else /* __KERNEL__ */

#define _GNU_SOURCE
#include <errno.h>
#include <pthread.h>
#include <getopt.h>
#include <sched.h>
#include <signal.h>
#include <stdarg.h>
#include <stdint.h>
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <time.h>
#include <math.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#include "cpuset.h"
#include "spinlock.h"
#include "threads.h"
#include "logging.h"

#define PRINT(fmt, args...)	\
	do{	\
		fprintf(stderr, fmt, ## args); \
} while (0)

#define DECLARE_ARGS(val, low, high)    unsigned low, high
#define EAX_EDX_VAL(val, low, high) ((low) | ((u64)(high) << 32))                                                      
#define EAX_EDX_ARGS(val, low, high)    "a" (low), "d" (high)
#define EAX_EDX_RET(val, low, high) "=a" (low), "=d" (high)

#define MIN(a,b) (((a)<(b))?(a):(b)) 
#define MAX(a,b) (((a)>(b))?(a):(b))

#define MAX_LOOPS_NR 1024
#define MAX_CPU_NR 64


typedef unsigned long long cycles_t;
typedef uint64_t u64;

static int j=0;
static int l=0;

char *program	= "";
const char optstring[] = "l:j:";
struct option options[] = {
	{ "",	required_argument,	0, 	'j'	},
	{ "",	required_argument,	0, 	'l'	},
	{ 0,	0,	0,	0 }
};

void usage(void)
{
	printf("usage: [-l loop_nr MAX %d] [-j lpj]\n",MAX_LOOPS_NR);
	printf("dmesg |grep lpj\n");
}

void help(void)
{
	usage();
}

#endif /* __KERNEL__ */

#define llrdtscll(val) \
	((val) = __llnative_read_tsc())             

static __always_inline unsigned long long __llnative_read_tsc(void)                                                      
{
    DECLARE_ARGS(val, low, high);

    asm volatile("rdtsc" : EAX_EDX_RET(val, low, high));                                                               

    return EAX_EDX_VAL(val, low, high);
}

static inline cycles_t llget_cycles(void)
{
	unsigned long long ret = 0;
	llrdtscll(ret);
	return ret;
}


struct per_cpu{
	char cpu_name[32];
	uint64_t delta[MAX_LOOPS_NR];
};

static struct per_cpu cpu_dat[MAX_CPU_NR];

/* simple loop based delay: */
static void delay_loop(unsigned long loops)
{
	asm volatile(
		"	test %0,%0	\n"
		"	jz 3f		\n"
		"	jmp 1f		\n"

		".align 16		\n"
		"1:	jmp 2f		\n"

		".align 16		\n"
		"2:	dec %0		\n"
		"	jnz 2b		\n"
		"3:	dec %0		\n"

		: /* we don't need output */
		:"a" (loops)
	);
}

/*
 * Since we calibrate only once at boot, this
 * function should be set once at boot and not changed
 */
static void (*delay_fn)(unsigned long) = delay_loop;

static void __ldelay(unsigned long loops)
{
	delay_fn(loops);
}

/*
 * Measure the number of TSC cycle it takes for a given amount of loop
 */
static void measure_tsc_cycle_per_loop(unsigned long lpj, int loop_nr)
{
	int x,cpu,warm;
	u64 t1, t2;
	struct per_cpu *pcpu;
	unsigned long flags;
	
#ifdef __KERNEL__
	cpu = raw_smp_processor_id();
#else /* __KERNEL__ */
	cpu = sched_getcpu();
	display_thread_sched_attr("");
#endif /* __KERNEL__ */

	PRINT("INFO %x / %x\n",lpj,loop_nr);
	pcpu = &cpu_dat[cpu];
	sprintf(pcpu->cpu_name,"cpu_%d",cpu);

	/* 
	 * By warming up we ensure the cache is warm
	 * and the CPU runs full throttle;
	 * NOTE that we only take the spinlock IRQ when the
	 * cpu is warm since the governor need IRQ to throttle...
	 */ 
	warm = 0;

again:
	for(x=0;x<loop_nr;x++){
		t1 = llget_cycles();
		__ldelay(lpj);
		t2 = llget_cycles();
		pcpu->delta[x] = t2-t1; 
	}
	if(warm != 2){
		warm ++;
		goto again;
	}

	// Serialize this operation
	PRINT("%s\n",pcpu->cpu_name);
	for(x=0;x<loop_nr;x++){
		PRINT("%8Lu\n",pcpu->delta[x]);
	}
}

#ifdef __KERNEL__

static int get_sample(void *unused)
{
	measure_tsc_cycle_per_loop(j,l);
	return 0;
}

static ssize_t read(struct file *file, char __user *buf,
			size_t count, loff_t *ppos)
{
	int i,err;
	struct cpumask mask;

/*
	for_each_online_cpu(i) {
		err = stop_machine(get_sample, &i, cpumask_of(1));
		if (err) {
			PRINT("Error stop_machine\n");
			return -1;
		}
	}
*/
	spin_lock_irq(&lock);
	get_sample(NULL);
	spin_unlock_irq(&lock);
	return 0;
}

static const struct file_operations fops = {
	.owner		= THIS_MODULE,
	.read		= read,
};

static struct miscdevice dev =
{
	.minor		= MISC_DYNAMIC_MINOR,
	.name		= "template",
	.fops		= &fops,
};

static int __init init(void)
{
	int retval,i;

	if(!j || !l){
		PRINT("PER CPU lpj info\n");
		for_each_online_cpu(i) {
			PRINT("CPU %d : lpj %lu\n",i,cpu_data(i).loops_per_jiffy);
		}
		return -1;
	}

	retval = misc_register(&dev);
	if (retval < 0)
		return retval;

	pr_info(DRIVER_DESC " version: " DRIVER_VERSION);
	return 0;
}

static void __exit cleanup(void)
{
	misc_deregister(&dev);
}

module_init(init);
module_exit(cleanup);

MODULE_VERSION(DRIVER_VERSION);
MODULE_LICENSE("GPL v2");
MODULE_AUTHOR(DRIVER_AUTHOR);
MODULE_DESCRIPTION(DRIVER_DESC);

#else /* __KERNEL__ */

int
main(int argc, char *argv[])
{
	int	c;
	int errs;
	extern int	opterr;
	extern int	optind;
	extern char	*optarg;

	if ((program = strrchr(argv[0], '/')) != NULL)
		++program;
	else
		program = argv[0];
	set_program_name(program);

	opterr = 0;
	errs = 0;
	while ((c = getopt_long(argc, argv, optstring, options, NULL)) != EOF) {
		switch (c) {
			case 'j':
				j = strtol(optarg, NULL, 0);
				break;
			case 'l':
				l = strtol(optarg, NULL, 0);
				if(l >= MAX_LOOPS_NR)
					++errs;
				break;
			default:
				ERROR(0, "unknown option '%c'", c);
				++errs;
				break;
		}
	}

	if (errs) {
		usage();
		exit(1);
	}

	if(!j || !l){
		usage();
		exit(1);
	}
		
	measure_tsc_cycle_per_loop(j,l);
}
#endif /* __KERNEL__ */


