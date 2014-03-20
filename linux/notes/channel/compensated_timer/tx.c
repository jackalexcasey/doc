/*
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
#include "clock.h"
#include "logging.h"
#include "atomic_64.h"

#define CPU_FREQ				2393715000

#define FREQ 60
#define PERIOD_CPU_CYCLE	CPU_FREQ/FREQ
#define MONOTONIC_PULSE_CYCLE_LENGTH	PERIOD_CPU_CYCLE/2

#define SEC_PERIOD	(1/FREQ) %1
#define NSEC_PERIOD (NSEC_PER_SEC/FREQ ) /* 1/FREQ * NSEC_PER_SEC */

#define JITTER_NSEC_PERIOD    200000
#define PAYLOAD_NSEC_PERIOD    100000

#define JITTER_PULSE_CYCLE_LENGTH (MONOTONIC_PULSE_CYCLE_LENGTH * JITTER_NSEC_PERIOD) / NSEC_PERIOD
#define PAYLOAD_PULSE_CYCLE_LENGTH (MONOTONIC_PULSE_CYCLE_LENGTH * PAYLOAD_NSEC_PERIOD) / NSEC_PERIOD

//#define PAYLOAD_PULSE_CYCLE_LENGTH MONOTONIC_PULSE_CYCLE_LENGTH

struct timespec carrier_ts = {
	.tv_sec = SEC_PERIOD,
	/* Here we provision for the timer jitter and the payload */
	.tv_nsec = NSEC_PERIOD - JITTER_NSEC_PERIOD - PAYLOAD_NSEC_PERIOD,
};

void tx(void)
{
	int x=0;
	int ret;
	cycles_t t0, t1, t2=0, delta=0;

	/*
	 * t0 mark the start of the cycle. The goal is to keep the system
	 * perfectly monotonic with respect to that t0.
	 *
	 * To achieve this goal we need some compensation. ! compensation is also subject to noise...
	 *  t1 -> t2 == [DATA start, DATA END] + NOISE during [DATA start, DATA END]
	 *  			+ NOISE [t1 to DATA start] + NOISE [ DATA end to t2 ]
	 *  t2 -> t1 == Loop RTT overhead
	 *
	 * The goal is to have t2 perfectly monotonic
	 */
	fprintf(stderr, "%Lu\n",PAYLOAD_PULSE_CYCLE_LENGTH);
	t0 = get_cycles();
	while(1){
		t1 = get_cycles();

		// DATA start
		calibrated_ldelay(PAYLOAD_PULSE_CYCLE_LENGTH-(delta/2));

		if(t2)
			delta = (t2 - t0)/2 - (x * PAYLOAD_PULSE_CYCLE_LENGTH);

		if(!(x%1000))
			fprintf(stderr, "%d %Ld\n", x, delta );
		x++;

		/*
		 * NOTE that the amount of noise we are subjected to depends
		 * directly on the amount of time spend from DATA start to DATA END
		 * Do we need to characterize the noise???
		 */

		// DATA end

		t2 = get_cycles();
	}
}

#if 0
void tx(void)
{
	int x;
	int ret;
	cycles_t t0, t1, t2, delta;

	x=1;
	t1 = 0;
	t2 = 0;

	fprintf(stderr, "%Lu %Lu\n",MONOTONIC_PULSE_CYCLE_LENGTH, get_cycles());

	t0 = get_cycles();
	while(1){
		t1 = get_cycles();
		ret = clock_nanosleep(CLOCK_MONOTONIC, TIMER_RELTIME, &carrier_ts, NULL);
		if(ret)
			DIE("clock_nanosleep");
		
		/* Calculate the real length of the sleep and account for the previous run */
		if(!t2)
			t2 = t1;
		delta = (t1 - t2)/2;
		//fprintf(stderr, "%Lu\n", delta);
		delta += (get_cycles() - t1)/2;
		//fprintf(stderr, "%Lu\n", delta);

		/* 
		 * TODO
		 * If the sleep period overflow in the payload period
		 * then error seek in the period and deliver payload[offset]
		 * If the sleep period overflow in the bext period
		 * the ripple out
		 */
		if(delta > (MONOTONIC_PULSE_CYCLE_LENGTH - PAYLOAD_PULSE_CYCLE_LENGTH) )
			DIE("### %Ld %Ld %Ld", delta, MONOTONIC_PULSE_CYCLE_LENGTH, PAYLOAD_PULSE_CYCLE_LENGTH);

		/*
		 * Then we trigger the padding LPJ to reach the exact value of 
		 * MONOTONIC_PULSE_CYCLE_LENGTH - PAYLOAD_PULSE_CYCLE_LENGTH
		 * where we can start data
		 */
		calibrated_ldelay((MONOTONIC_PULSE_CYCLE_LENGTH - PAYLOAD_PULSE_CYCLE_LENGTH) - delta);

		// DATA start
		// ...
		// ...
		// DATA end
		calibrated_ldelay(PAYLOAD_PULSE_CYCLE_LENGTH);

		/* 
		 * Here we have reached the exact value of MONOTONIC_PULSE_CYCLE_LENGTH
		 */
		t2 = get_cycles();

		/* Debugging this is showing that there is a drift */
		if(!(x%100))
			fprintf(stderr, "%d %Lu\n", x, (t2 - t0)/2 - (x * MONOTONIC_PULSE_CYCLE_LENGTH));
		x++;	

	}
}
#endif

