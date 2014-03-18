#if 0
/*
 * Author: Etienne Martineau <etmartin@cisco.com>
 *
 * This work is licensed under the terms of the GNU GPL, version 2.
 *
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

extern int transmitter;
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
#define TIMER_AVG_JITTER	100*1000 /* 100 uSec in average */

const struct timespec carrier_ts = {
	.tv_sec = V_SYNC_SEC_PERIOD,
	.tv_nsec = V_SYNC_NSEC_PERIOD,
};

const struct timespec carrier_adj_ts = {
	.tv_sec = V_SYNC_SEC_PERIOD,
	.tv_nsec = V_SYNC_NSEC_PERIOD - TIMER_AVG_JITTER,
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



/*
 *
 * Background:
 *  Like AM/FM radio here we have a transmitter that does the modulation and a 
 *  receiver that does the de-modulation.
 *
 *  The power of the transmitter is related to the amount of CPU cycle it 
 *  burns i.e. the CFS vruntime. The transmitter can be in low power mode 
 *  ( barely visible by CFS ) and high power mode for maximum throughput.
 *  The 'stealth' factor is directly related to the amount of power available
 *  for communication.
 *
 *  The receiver can operate is high power mode all the time since this 
 *  end belong to us
 *
 *  The signal we modulate is AKIN to a TV monitor i.e.:
 *   carrier
 *   vsync, [hsync:data,data,data...], [hsync:data,data,data...], ..., v_sync
 *
 * Simulation:
 *  The end goal is to use an execution stream that modulate contention in a
 *  SMT pipeline. The detection of that contention is achieve by measuring the
 *  time it takes for a given execution stream to execute. 
 *
 *  For modelization purpose we modulate a shared memory variable ( spinlock ).
 *  The detection of the contention is achieve by reading the variable's value.
 *  RD and WR are atomic on x86_64.
 *
 * Synchronization & communication :
 *  The first step is to establish basic synchronization this is done by the
 *  carrier detection logic. Transmitter stays in low power mode till the
 *  synchronization is establish.
 *
 *  Transmittter trigger a carrier at regular interval using a timer. During
 *  this time the transmitter stay in low power mode to avoid detection.
 *  Right after the carrier, the transmitter probe the channel to see if there
 *  is a receiver on the other end. If there is then the transmitter will start
 *  sending out data.
 *
 *
 *  Initially the receiver aggressively probes the channel at high frequence to
 *  detect any possible carrier.  As soon as a carrier is detected the receive enter in timer mode
 *  in order to reduce power consumption. The trick here is that since we know that 
 *  there is a lots of jitter on timer we wake up earlier and probe
 *  manually the channel using a TSC projection.
 *
		 *  From cyclictest we know that the jitter for servicing a timer is in
		 *  the order of ~100uSec. Here we can either try to adapt/compensate OR
		 *  rely on the fact that the other end have the same noise distribution
		 *  pattern which at the end cancel each other.
 *
 *  Part of carrier detection logic, the transmitter send out a vsync() signal
 *  The vsync signal is a 10 usec ramp in a 16.666 msec (60 HZ) duty cycle.

 *
 *  At first, the receiver tries to detect the ramp by doing a coarse tunning 
 *  ( shifting 1/50 of 16.666 msec at every cycle it doesn't find it) then it 
 *  fall on the fine grain locking scheme which feeds its retro-action loop 
 *  using the amplitude of the ramp itself.
 *
 *  The fine gr
 *  The vsync() signal's frequence is 60 HZ 
 *
 */

void do_carrier(void)
{
	int x;
	for(x=1; x<100; x++){
		*spinlock = x;
	}
	*spinlock = 0;
}

int detect_carrier(void)
{
	int x;

	// Add the fast forward logic as well
	for(x=1; x<100; x++){
		if(*spinlock)
			return;
	}
}

void relax_cpu(const struct timespec *request)
{
	int ret;
	static int x=0;

	ret = clock_nanosleep(CLOCK_MONOTONIC, TIMER_RELTIME, request, NULL);
	if(ret)
		DIE("clock_nanosleep");
	if(!(x%V_SYNC_HZ))
		fprintf(stderr, ".");
	x++;
}

int tx_data(char *payload)
{
	int state = 0;
	int x;

	while(1){
		switch (state){
		case 0: /* Carrier modulation */
			relax_cpu(&carrier_ts);
			do_carrier();
			if(detect_carrier()){
				state = 1;
				break;
			}
		break;
		}
	}
}

char *rx_data(void)
{
	int state = 0;
	int x;

	while(1){
		switch (state){
		case 0: /* Carrier detection */
			if(detect_carrier()){
				state = 1;
				break;
			}
		break;
		case 1:
			fprintf(stderr, " Carrier detected");
			break;
		}
	}
}

void transmitter_loop(void)
{
	char dat[] = "fsdfsdf";
	tx_data(dat);
}

void receiver_loop(void)
{
	rx_data();
}
#endif

