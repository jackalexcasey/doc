/*
 * bogomips.c - Spinloop calibration
 *
 * Copyright (C) 2010 Cisco Systems
 * Author: Etienne Martineau <etmartin@cisco.com>
 *
 * This work is licensed under the terms of the GNU GPL, version 2.
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
#include <sys/mman.h>
#include <fcntl.h>
#include <unistd.h>

#include "cpuset.h"
#include "spinlock.h"
#include "threads.h"
#include "logging.h"
#include "atomic_64.h"

#define PRINT(fmt, args...)	\
	do{	\
		fprintf(stderr, fmt, ## args); \
} while (0)

# define DIE(format, ...) do {\
	fprintf(stderr, "Fatal %s %d " format,__FILE__, __LINE__, ## __VA_ARGS__);\
	exit(1);\
}while(0)

int leader = 0;
volatile int *spinlock = NULL;
char *program	= "";
const char optstring[] = "c:l";
struct option options[] = {
	{ "",	required_argument,	0, 	'j'	},
	{ "",	required_argument,	0, 	'l'	},
	{ 0,	0,	0,	0 }
};
#define TIMER_RELTIME       0

/*
 * Basic definition
 */
#define USEC_PER_SEC	1000000
#define NSEC_PER_SEC	1000000000

/* V sync is 60HZ => 16666666 nsec period*/
#define V_SYNC_HZ			60
#define V_SYNC_SEC_PERIOD	16666666 / NSEC_PER_SEC
#define V_SYNC_NSEC_PERIOD	16666666 % NSEC_PER_SEC

const struct timespec v_sync_ts = {
	.tv_sec = V_SYNC_SEC_PERIOD,
	.tv_nsec = V_SYNC_NSEC_PERIOD,
};

void usage(void)
{
	printf("usage: [-c cpu sets] [-l leader mode] \n");
}

void help(void)
{
	usage();
}

/*
 * This hides the detail about the method used for communication.
 * For modelization we used shared memory variable.
 * For real implementation we use SMT pipeline contention. In that
 * case this function takes care to set the affinity of the HT siblings...
 */
void open_channel(void)
{
	int fd;
	void *ptr;

	if ((fd = shm_open("channel", O_CREAT|O_RDWR,
					S_IRWXU|S_IRWXG|S_IRWXO)) > 0) {
		if (ftruncate(fd, 1024) != 0)
			DIE("could not truncate shared file\n");
	}
	else
		DIE("Open channel");
	
	ptr = mmap(NULL,1024,PROT_READ|PROT_WRITE,MAP_SHARED,fd,0);
	if(ptr == MAP_FAILED)
		DIE("mmap");

	spinlock = ptr; /* spinlock is the first object in the mmap */
	if(leader)
		*spinlock = 0;
	return;
}




#if 0
void detect_carrier(void)
{
	int x;

	x=0;
	while(1){
		/* we have to measure timestamp also to make sure we see 100 hops in the
		 * required amount of time
		 */
		/* Here we wait for the rising edge */
		while(*spinlock == 0)
			usleep(1);
		/* Here there is a 1 state */
		usleep(1);
		while(*spinlock == 1);
		/* Here there is a 1 state */
		usleep(2);
		x++;
		if(x==100){
			fprintf(stderr, ".");
			x=0;
		}
	}
}
#endif

void detect_v_sync(void)
{
	int x, y, z, t, ret, v1, v2, conv;
	struct timespec pll_v_sync_ts;

	pll_v_sync_ts.tv_sec = V_SYNC_SEC_PERIOD;
	pll_v_sync_ts.tv_nsec = V_SYNC_NSEC_PERIOD;

	x=0;
	t=0;
	while(1){
		z=0;

		ret = clock_nanosleep(CLOCK_MONOTONIC, TIMER_RELTIME, &pll_v_sync_ts, NULL);
		if(ret)
			DIE("clock_nanosleep");
		/* reset to original delay */
		pll_v_sync_ts.tv_nsec = V_SYNC_NSEC_PERIOD;

		/* 
		 * Here we have the double sampling rate
		 * We need to know how long this is executing to perform the convergence
		 */
		for(y=0;y<10;y++){
			v1 = *spinlock;
			usleep(2);
			v2 = *spinlock;
			usleep(2);
			if(v1 != v2)
				z++;
		}
		if(z == 0)
			conv = 50;
		else if(z < 2)
			conv = 100;
		else if(z<4)
			conv = 200;
		else if(z<6)
			conv = 400;
		else
			conv = 0;
		if(conv)
			pll_v_sync_ts.tv_nsec = pll_v_sync_ts.tv_nsec - (pll_v_sync_ts.tv_nsec/conv);

	//NEED a SW PLL
//		fprintf(stderr, "%d ",conv);
		
		t=t+z;
		if(!(x%V_SYNC_HZ)){
			fprintf(stderr, "#%d",t);
			t=0;
		}
		x++;
	}
}

void trigger_v_sync(void)
{
	int x, y, ret;

	x=0;
	while(1){
		ret = clock_nanosleep(CLOCK_MONOTONIC, TIMER_RELTIME, &v_sync_ts, NULL);
		if(ret)
			DIE("clock_nanosleep");

		for(y=0;y<10;y++){
			*spinlock = 1;
			usleep(2);
			*spinlock = 0;
			usleep(2);
		}

		if(!(x%V_SYNC_HZ))
			fprintf(stderr, ".");
		x++;
	}
}


void sniffer_loop(void)
{
	while(1){
		detect_v_sync();
	}
}

void leader_loop(void)
{
	while(1){
		trigger_v_sync();
	}
}

void * worker_thread(void *arg)
{
	int cpu;

	cpu = sched_getcpu();
	PRINT("worker_thread start on CPU %d in %s\n",cpu ,leader ? "Leader mode":"Slave mode");
	if(leader)
		leader_loop();
	else
		sniffer_loop();

	return NULL;
}

/*
interval is usec
 589     interval.tv_sec = par->interval / USEC_PER_SEC;
  590     interval.tv_nsec = (par->interval % USEC_PER_SEC) * 1000;
  struct timespec now, next, interval,
 */

/*
 *
 * Here we have a execution stream that modulate contention in a SMT pipeline.
 * The detection of that contention is achieve by measuring the time it takes
 * for an execution stream to execute. 
 *
 * The leader does the modulation
 * The sniffer does the detection. This is a one way channel
 *
 * For modelization purpose we modulate a shared memory variable ( spinlock ).
 * The detection of the contention is achieve by reading the variable's value.
 * The leader does WR and the sniffer does RD
 * RD and WR are atomic on x86_64.
 *
 * The signal we modulate is AKIN to a TV monitor i.e.:
 *  V_sync, [Hsync:data,data,data...], [Hsync:data,data,data...], ..., V_sync
 *
 * The frequency of V_sync is 60HZ
 */
int
main(int argc, char *argv[])
{
	int	c;
	int errs;
	int	ncpus;
	int	nthreads;
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
			case 'l':
				leader = 1;
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

	/*
	 * Open the comm channel
	 */
	open_channel();

	/*
 	 * create the threads
 	 */
	ncpus = count_cpus(&cpus);
	nthreads = create_per_cpu_threads(&cpus, worker_thread, NULL);
	if (nthreads != ncpus) {
		ERROR(0, "failed to create threads: expected %d, got %d",
			ncpus, nthreads);
		if (nthreads) {
			join_threads();
		}
		return 1;
	}
	join_threads();
}
