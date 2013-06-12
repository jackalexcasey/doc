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
#include <linux/seq_file.h>
#include <linux/debugfs.h>
#include <linux/kthread.h>

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
static struct dentry *debugfs_file;

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

#define seq_printf(file,fmt, args...) \
	do{ \
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

struct seq_file{
};

static int j=0;
static int l=0;
static int Gfd;
static int p=0;
static int looping=0;

char *program	= "";
const char optstring[] = "l:j:c:kp:L:";
struct option options[] = {
	{ "",	required_argument,	0, 	'j'	},
	{ "",	required_argument,	0, 	'l'	},
	{ 0,	0,	0,	0 }
};

void usage(void)
{
	printf("usage: [-l loop_nr MAX %d] [-j lpj] [-c cpu sets] [-k kernel data] \
		[-p pause (msec) ] [-L constant loop]\n",MAX_LOOPS_NR);
	printf("dmesg |grep lpj\n");
}

void help(void)
{
	usage();
}

#define rdtscll(val) \
	((val) = __native_read_tsc())             

static __always_inline unsigned long long __native_read_tsc(void)                                                      
{
    DECLARE_ARGS(val, low, high);

    asm volatile("rdtsc" : EAX_EDX_RET(val, low, high));                                                               

    return EAX_EDX_VAL(val, low, high);
}

static inline cycles_t get_cycles(void)
{
	unsigned long long ret = 0;
	rdtscll(ret);
	return ret;
}
#endif /* __KERNEL__ */

/*
 * In vector mode an entry is taken for value exceding the threshold only
 * In non-vector mode, there is an entry for each value 
 */
#undef __VECTOR_MODE__
#define __VECTOR_THRESHOLD__ 200032

/*
 * In scope view the data are printed in square wave format output.
 * In non-scope mode, the entry is printed with it's time stamp
 */
#undef __SCOPE_VIEW__

struct per_cpu{
	char cpu_name[32];
	uint64_t time[MAX_LOOPS_NR];
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

#if 0
/*
 * To measure the effect of rdtsc when suspending the VM during the 
 * 5 sec window
 */
static int measure_tsc_cycle_per_loop(void *arg)
{
	u64 t0,t1,t2,t3,t4;
	while(1){
		t1 = get_cycles();
		fprintf(stderr,"!\n");
		sleep(5);
		fprintf(stderr,"#");
		t2 = get_cycles();
		fprintf(stderr,"Delta %Lu\n",t2-t1);
	}
}
#endif

#if 0
/*
 * To measure the effect of rdtsc when emulating
 */
volatile unsigned long z=0;
static int measure_tsc_cycle_per_loop(void *arg)
{
	int x,y;
	u64 t0,t1,t2,t3,t4;

	for(y=0;y<10;y++){
		t1 = get_cycles();
		for(x=0;x<100000;x++){
//			z++;
			t3 = get_cycles();
		}
		t2 = get_cycles();
		fprintf(stderr,"Delta %Lu\n",t2-t1);
	}
	exit(0);
}
#endif

#ifdef __KERNEL__
static int measure_tsc_cycle_per_loop(void *arg)
#else
static void * measure_tsc_cycle_per_loop(void *arg)
#endif
{
	int x,y,z,cpu;
	u64 t0,t1,t2,t3,t4;
	struct per_cpu *pcpu;
	unsigned long lpj = j;
	int loop_nr = l;
	struct timespec req, rem;

#ifdef __KERNEL__
	cpu = raw_smp_processor_id();
	pcpu = &cpu_dat[cpu];
	sprintf(pcpu->cpu_name,"Kernel_cpu_%d",cpu);
#else /* __KERNEL__ */
	if(p){
		req.tv_sec = p/1000;
		req.tv_nsec = ((p-(req.tv_sec*1000))*1000*1000);
	}
	cpu = sched_getcpu();
	//display_thread_sched_attr("");
	pcpu = &cpu_dat[cpu];
	sprintf(pcpu->cpu_name,"User_cpu_%d",cpu);
#endif /* __KERNEL__ */

	/* 
	 * By warming up we ensure the cache is warm
	 * and the CPU runs full throttle;
	 * NOTE that we only take the spinlock IRQ when the
	 * cpu is warm since the governor needs IRQ to throttle...
	 * This is not needed since we turn off cpu throttling in BIOS
	 */ 
#ifdef __KERNEL__
	local_irq_disable();
	preempt_disable();
#endif
	t0 = get_cycles();

#ifndef ___VECTOR_MODE__
	for(x=0;x<loop_nr;x++){
#else
	for(y=0;y<loop_nr;){
#endif
		t1 = get_cycles();
		if(!x)
			t3 = 0;
		else
			t3 = t1 - t2;
		__ldelay(lpj);
		t2 = get_cycles();
		t4 = (t2-t1 + t3);

#ifndef ___VECTOR_MODE__
		pcpu->delta[x] = t4;
#else
		//We trick the flow to minimize the jitter across the 2 path
		pcpu->time[y] = t1-t0;
		pcpu->delta[y] = t4  & (~0x1f);
		if( t4 != __VECTOR_THRESHOLD__)
			y++;
#endif

	}
#ifndef __KERNEL__
	if(p){
		if(nanosleep(&req, &rem)){
			perror("");
			exit(-1);
		}
	}
#endif
#ifdef __KERNEL__
	local_irq_enable();
	preempt_enable();
	return 0;
#else
	return NULL;
#endif
}

#ifdef __KERNEL__
static int tsc_open(struct inode *inode, struct file *filep)
{
	return 0;
}

static ssize_t tsc_read(struct file *filep, char __user *buf,
	size_t count, loff_t *ppos)
{
	unsigned long p = *ppos;
	ssize_t read, sz;
	char *ptr,*dat;

	if( (*ppos+count) > (sizeof(struct per_cpu) * MAX_CPU_NR))
		return -EFAULT;

	read = 0;

	while (count > 0) {
		/*
		 * Handle first page in case it's not aligned
		 */
		if (-p & (PAGE_SIZE - 1))
			sz = -p & (PAGE_SIZE - 1);
		else
			sz = PAGE_SIZE;

		sz = min_t(unsigned long, sz, count);
		dat = (char*)cpu_dat;
		ptr = &dat[p];

		if (copy_to_user(buf, ptr, sz)) {
			return -EFAULT;
		}

		buf += sz;
		p += sz;
		count -= sz;
		read += sz;
	}

	*ppos += read; //Seek the file
	return read;
}

static ssize_t tsc_write(struct file * filep, const char __user * buf,
	size_t count, loff_t *ppos)
{
	/* Run the measurement on this CPU */
	measure_tsc_cycle_per_loop(NULL);
	return count;
}

static const struct file_operations fops = {
	.owner		= THIS_MODULE,
	.read		= tsc_read,
	.write		= tsc_write,
	.open		= tsc_open,
};

static int __init init(void)
{
	int i;

	if(!j || !l){
		PRINT("PER CPU lpj info\n");
		for_each_online_cpu(i) {
			PRINT("CPU %d : lpj %lu\n",i,cpu_data(i).loops_per_jiffy);
		}
		return -1;
	}

	memset(cpu_dat,0,sizeof(cpu_dat));

	debugfs_file = debugfs_create_file("spinloop", S_IFREG | S_IRUGO, NULL, NULL, &fops);
	if(!debugfs_file)
		return -1;

	pr_info(DRIVER_DESC " version: " DRIVER_VERSION);
	return 0;
}

static void __exit cleanup(void)
{
	debugfs_remove(debugfs_file);
}

module_init(init);
module_exit(cleanup);

MODULE_VERSION(DRIVER_VERSION);
MODULE_LICENSE("GPL v2");
MODULE_AUTHOR(DRIVER_AUTHOR);
MODULE_DESCRIPTION(DRIVER_DESC);

#else /* __KERNEL__ */

/*
 * Be carefull this is an iterator
 */
static int tsc_show(struct seq_file *m, void *p)
{
	int x,y;
	struct per_cpu *pcpu;
	static init = 0;

	if(!init){
		for(y=0;y<MAX_CPU_NR;y++){
			pcpu = &cpu_dat[y];
			if(!strlen(pcpu->cpu_name))
				continue;
			seq_printf(m, "%s;",pcpu->cpu_name);
		}
		seq_printf(m, "\n",pcpu->cpu_name);
		init = 1;
	}


	for(x=0;x<l;x++){
		for(y=0;y<MAX_CPU_NR;y++){
			pcpu = &cpu_dat[y];
			if(!strlen(pcpu->cpu_name))
				continue;
			if(!pcpu->delta[x])
				continue;
#ifdef __SCOPE_VIEW__
			seq_printf(m, "%Lu,1\n",pcpu->time[x]);
			seq_printf(m, "%Lu,0\n",pcpu->time[x]);
			seq_printf(m, "%Lu,0\n",pcpu->time[x]+pcpu->delta[x]);
			seq_printf(m, "%Lu,1\n",pcpu->time[x]+pcpu->delta[x]);
		}
#else
			seq_printf(m, "%Lu,%Lu;",pcpu->time[x], pcpu->delta[x]);
		}
		seq_printf(m, "\n");
#endif
	}
	if(!init)
		seq_printf(m, "\n");

	return 0;
}

/*
 *
 * This thread write to sysfs which internally
 * trigger the measurements per CPU
 *
 * This is easier than creating kthread and managing the cpu sets...
 */
static void * dump_kernel(void *arg)
{
	int val;
	if(write(Gfd, &val,sizeof(int))<0)
		exit(1);
	return NULL;
}

int
main(int argc, char *argv[])
{
	int	c;
	int errs;
	int	ncpus;
	int	nthreads;
	int kernel=0;
	cpu_set_t	cpus;
	extern int	opterr;
	extern int	optind;
	extern char	*optarg;
	
	if ((program = strrchr(argv[0], '/')) != NULL)
		++program;
	else
		program = argv[0];
	set_program_name(program);

	/*
	 * default to checking all cpus
	 */
	for (c = 0; c < CPU_SETSIZE; c++) {
		CPU_SET(c, &cpus);
	}

	opterr = 0;
	errs = 0;

	while ((c = getopt_long(argc, argv, optstring, options, NULL)) != EOF) {
		switch (c) {
			case 'c':
				if (parse_cpu_set(optarg, &cpus) != 0)
					++errs;
				break;
			case 'k':
				kernel = 1;
				break;
			case 'j':
				j = strtol(optarg, NULL, 0);
				j = j - 6000/2; /* Remove a constanc offset */
				break;
			case 'L':
				looping = strtol(optarg, NULL, 0);
				break;
			case 'p':
				p = strtol(optarg, NULL, 0);
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

	/*
	 * limit the set of CPUs to the ones that are currently available
	 * (Note that on some kernel versions sched_setaffinity() will fail
	 * if you specify CPUs that are not currently online so we ignore
	 * the return value and hope for the best)
	 */
	sched_setaffinity(0, sizeof cpus, &cpus);
	if (sched_getaffinity(0, sizeof cpus, &cpus) < 0) {
		ERROR(errno, "sched_getaffinity() failed");
		exit(1);
	}

again:
	memset(cpu_dat,0,sizeof(cpu_dat));

	/*
 	 * create the threads
 	 */
	ncpus = count_cpus(&cpus);

	if(kernel){
		Gfd = open("/sys/kernel/debug/spinloop", O_RDWR);
		if(Gfd <0 )
			exit(1);
		nthreads = create_per_cpu_threads(&cpus, dump_kernel, NULL);
	}
	else
		nthreads = create_per_cpu_threads(&cpus, measure_tsc_cycle_per_loop, NULL);
	if (nthreads != ncpus) {
		ERROR(0, "failed to create threads: expected %d, got %d",
			ncpus, nthreads);
		if (nthreads) {
			join_threads();
		}
		return 1;
	}
	join_threads();

	if(kernel){
		char *dat;
		int size;
		dat = (char*)cpu_dat;
		size = read(Gfd, dat, (sizeof(struct per_cpu) * MAX_CPU_NR));
		if(size <0)
			exit(-1);
	}

	tsc_show(NULL,NULL);
	if(looping>1){
		looping--;
		goto again;
	}
}
#endif /* __KERNEL__ */


