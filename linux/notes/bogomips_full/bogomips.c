/*
 * Copyright 2008 Google Inc. All Rights Reserved.
 * Author: md@google.com (Michael Davidson)
 *
 * Based on time-warp-test.c, which is:
 * Copyright (C) 2005, Ingo Molnar
 */
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

#define HZ 1000
#define NSEC_PER_MSEC 1000*1000 
#define MAX_CPUS CPU_SETSIZE

#define MIN(a,b) (((a)<(b))?(a):(b)) 
#define MAX(a,b) (((a)>(b))?(a):(b))

char	*program	= "";
const char optstring[] = "j:t:c:h";

unsigned long lpj=100000000;
volatile int term =1;
int tt = 10;

struct option options[] = {
	{ "cpus",	required_argument,	0, 	'c'	},
	{ "lpj",	required_argument,	0, 	'j'	},
	{ "time",	required_argument,	0, 	't'	},
	{ "help",	no_argument,		0, 	'h'	},
	{ 0,	0,	0,	0 }
};

void usage(void)
{
	printf("usage: [-h] [-c <cpu_set>] [-j <lpj>] [-t <time out>]");
	printf("dmesg |grep lpj\n");
}

const char help_text[] =
"check time sources for monotonicity across multiple CPUs\n"
"  -c,--cpus        set of cpus to test (default: all)\n";


void help(void)
{
	usage();
}

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
 * get the TSC as 64 bit value with CPU clock frequency resolution
 */
#if defined(__x86_64__)
static inline uint64_t rdtsc(void)
{
	uint32_t	tsc_lo, tsc_hi;
	__asm__ __volatile__("rdtsc" : "=a" (tsc_lo), "=d" (tsc_hi));
	return ((uint64_t)tsc_hi << 32) | tsc_lo;
}
#elif defined(__i386__)
static inline uint64_t rdtsc(void)
{
	uint64_t	tsc;
	__asm__ __volatile__("rdtsc" : "=A" (tsc));
	return tsc;
}
#else
#error "rdtsc() not implemented for this architecture"
#endif

static inline uint64_t rdtsc_mfence(void)
{
	__asm__ __volatile__("mfence" ::: "memory");
	return rdtsc();
}


static inline uint64_t rdtsc_lfence(void)
{
	__asm__ __volatile__("lfence" ::: "memory");
	return rdtsc();
}

/* Kernel Way of reading the TSC */
typedef uint64_t u64;

#define DECLARE_ARGS(val, low, high)    unsigned low, high
#define EAX_EDX_VAL(val, low, high) ((low) | ((u64)(high) << 32))
#define EAX_EDX_RET(val, low, high) "=a" (low), "=d" (high)
static __always_inline unsigned long long __native_read_tsc(void)                                                      
{
    DECLARE_ARGS(val, low, high);

    asm volatile("rdtsc" : EAX_EDX_RET(val, low, high));                                                               

    return EAX_EDX_VAL(val, low, high);
}

#define rdtscll(val) \
	((val) = __native_read_tsc())             

struct data{
	uint64_t delta;
};

struct percpu{
	char cpu_name[32];
	struct data dat[1024*32];
};

struct percpu *all_cpu[MAX_CPUS];

void * calibrate_loop(void *arg)
{
	uint64_t tsc=0,tsc_new;
	int x;
	int cpu = sched_getcpu();
	struct percpu *pcpu = all_cpu[cpu];

	display_thread_sched_attr("");
	sprintf(pcpu->cpu_name,"_%d",cpu);

//TODO put a thread synchro to start all of them at the same time...
//Just an atomic variable
	sleep(1);

	x=0;
	while(term){
		//tsc = rdtsc_lfence();
		tsc = rdtsc();
		//rdtscll(tsc);
		__ldelay(lpj);
		//tsc_new = rdtsc_lfence();
		tsc_new = rdtsc();
		//rdtscll(tsc_new);
		pcpu->dat[x].delta = tsc_new-tsc; 
//		fprintf(stderr,".");
		x++;
	}

	return NULL;
}

void catch_alarm(int sig)
{
    term =0;
}

void f1(void)
	{
		// This code output every cpu run in seperate file
		char dat[256];
		int fd;
		int offset;
		int y,x;
		struct percpu *pcpu;
	
		offset = 0;
		offset += sprintf(&dat[offset],"./nr_cpu");
		for(x=0; x<64; x++){
			if(!all_cpu[x])
				continue;
			pcpu = all_cpu[x];
			offset += sprintf(&dat[offset],"_%s",pcpu[y].cpu_name);
		}
		offset += sprintf(&dat[offset],".csv");

		unlink(dat);
		fd = open(dat,O_CREAT|O_RDWR|O_EXCL,0777);
		if(fd<0){
			perror("");
			exit(1);
		}

		for(y=0;y<1;y++){
			offset = 0;
			for(x=0; x<64; x++){
				if(!all_cpu[x])
					continue;
				pcpu = all_cpu[x];
				offset += sprintf(&dat[offset],"%s;",pcpu[y].cpu_name);
			}
			write(fd,dat,strlen(dat));
			sprintf(dat,"\n");
			write(fd,dat,strlen(dat));
		}

		while(1){
			offset = 0;
			for(x=0; x<64; x++){
				if(!all_cpu[x])
					continue;
				pcpu = all_cpu[x];
				if(pcpu->dat[y].delta ==0 )
					goto out;
				offset += sprintf(&dat[offset],"%8Lu;",pcpu->dat[y].delta);
					//(pcpu->dat[y].delta*1000)/lpj);
			}
			write(fd,dat,strlen(dat));
			sprintf(dat,"\n");
			write(fd,dat,strlen(dat));
			y++;
		}
out:
		close(fd);
	}

/*
 * Bu disabling the tsc; notsc we have the following lpj
 *  237 [    0.004000] Calibrating delay loop... 1723.39 BogoMIPS (lpj=3446784)
 */

int
main(int argc, char *argv[])
{
	int		ncpus;
	int		nthreads;
	int		c;
	cpu_set_t	cpus;
	int		errs;
	extern int	opterr;
	extern int	optind;
	extern char	*optarg;
	int cpu;

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
			case 'j':
				lpj = strtol(optarg, NULL, 0);
				break;
			case 't':
				tt = strtol(optarg, NULL, 0);
				break;
			case 'h':
				help();
				exit(0);
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

	for (cpu = 0; cpu < MAX_CPUS; cpu++) {
		if (!CPU_ISSET(cpu, &cpus)){
			all_cpu[cpu] = NULL;
			continue;
		}
		all_cpu[cpu] = malloc(sizeof(struct percpu));
		if(!all_cpu[cpu]){
			ERROR(errno, "malloc failed");
			exit(1);
		}
		memset(all_cpu[cpu], 0, sizeof(struct percpu));
	}

	/*
 	 * create the threads
 	 */
	ncpus = count_cpus(&cpus);
	nthreads = create_per_cpu_threads(&cpus, calibrate_loop, NULL);
	if (nthreads != ncpus) {
		ERROR(0, "failed to create threads: expected %d, got %d",
			ncpus, nthreads);
		if (nthreads) {
			join_threads();
		}
		return 1;
	}
    signal(SIGALRM, catch_alarm);
    alarm(tt);

	join_threads();

	f1();
}

