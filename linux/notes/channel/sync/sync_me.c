/*
 * Author: Etienne Martineau <etmartin@cisco.com>
 *
 * This work is licensed under the terms of the GNU GPL, version 2.
 *
 * Background:
 *
 * Here we have a execution stream that modulate contention in a SMT pipeline.
 * The detection of that contention is achieve by measuring the time it takes
 * for an execution stream to execute. 
 *
 * The sender does the modulation
 * The sniffer does the detection. This is a one way channel
 *
 * For modelization purpose we modulate a shared memory variable ( spinlock ).
 * The detection of the contention is achieve by reading the variable's value.
 * The sender does WR and the sniffer does RD
 * RD and WR are atomic on x86_64.
 *
 * The signal we modulate is AKIN to a TV monitor i.e.:
 *  V_sync, [Hsync:data,data,data...], [Hsync:data,data,data...], ..., V_sync
 *
 *  The one problem is that is we burn too much CPU CFS will flag us
 *
 * The frequency of V_sync is 60HZ
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

#define TIMER_RELTIME 0

#define PRINT(fmt, args...)	\
	do{	\
		fprintf(stderr, fmt, ## args); \
} while (0)

# define DIE(format, ...) do {\
	fprintf(stderr, "Fatal %s %d " format,__FILE__, __LINE__, ## __VA_ARGS__);\
	exit(1);\
}while(0)

extern int sender;
extern volatile int *spinlock;

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

void detect_v_sync(void)
{
	int lock =0;
	int x, y, z, t, ret, v1;
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
		/* always make sure to have the original delay by default */
		pll_v_sync_ts.tv_nsec = V_SYNC_NSEC_PERIOD;

		for(y=1;y<11;y++){
			v1 = *spinlock;
			usleep(1);
			/*
			 * Here we use this auto-adjusting trick to nail down on the 
			 * synchronization. This is the fine trim.
			 * In real mode the same technique can be applied
			 */
			if(v1){
				if(!lock)
					y=v1;
				z++;
			}
		}
		/*
		 * If we detect that we are off the sync then we try to catch up at the
		 * coarse level by trimming 1/50 of the period
		 */
		if(!z){
			fprintf(stderr, ".");
			pll_v_sync_ts.tv_nsec = pll_v_sync_ts.tv_nsec - (pll_v_sync_ts.tv_nsec/50);
		}
		
		t=t+z;
		if(!(x%V_SYNC_HZ)){
			if(t){
				fprintf(stderr, "Locked %d\n",t);
			/*
			 * This is an experiment;
			 * Once we reach the locking state i.e. t >600 we disable the retro 
			 * feedback part of the the spinloop
			 * The net effect is that we see lots of drifting which
			 * at the end is expected.
			 * With the retrofeedback the locking is maintain solid at 600
			 * This means that we will need similar retro feedback in the execution
			 * pipeline
			 */
				if(t>=600)
					lock=1;
			}
			t=0;
		}
		x++;
	}
}

void sniffer_loop(void)
{
	while(1){
		detect_v_sync();
	}
}

// WE casn try to do few things:
// A) come with a wavw form generator using this technique
// This one would pin down the CPU all together...
// OR use the cache for the payload inter-vsync...
//   Frame#1 frame #2 ( flip flop )
//
//
void trigger_v_sync(void)
{
	int x, y, ret;

	x=0;
	while(1){
		/*
		 *  From cyclictest we know that the jitter for servicing a timer is in
		 *  the order of ~100uSec. Here we can either try to adapt/compensate OR
		 *  rely on the fact that the other end have the same noise distribution
		 *  pattern which at the end cancel each other.
		 */
		ret = clock_nanosleep(CLOCK_MONOTONIC, TIMER_RELTIME, &v_sync_ts, NULL);
		if(ret)
			DIE("clock_nanosleep");

		/*
		 * Here we generate the v_sync signal;
		 * In simulation we do that by incrementing a shared memory 
		 * variable during a predefined period of time.
		 * In real mode we would create a increasing contention pattern
		 * over that period of time.
		 */
		for(y=1;y<11;y++){
			*spinlock = y;
			usleep(1);
		}
		*spinlock = 0;

		if(!(x%V_SYNC_HZ))
			fprintf(stderr, ".");
		x++;
	}
}

void sender_loop(void)
{
	while(1){
		trigger_v_sync();
	}
}
