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

#ifndef __CHARACTERIZATION__

#define CPU_FREQ								2393715000

#define JITTER_NSEC_PERIOD (cycles_t)			200000
#define JITTER_CYCLE_LENGTH (cycles_t)			478743
/* 
 * This is the duty cycle of the PAYLOAD in nsec
 * PAYLOAD_PULSE_NSEC = ((1/FRAME_FREQ) * NSEC_PER_SEC)
 */
#define FRAME_FREQ (cycles_t)					60
//#define PAYLOAD_PULSE_NSEC (cycles_t) 			(NSEC_PER_SEC/FRAME_FREQ) 
#define PAYLOAD_PULSE_NSEC (cycles_t) 			8355213

/* 
 * This is the duty cycle of the PAYLOAD in bus cycle 
 * (CPU_FREQ * (1/FRAME_FREQ))/2
 */
//#define PAYLOAD_PULSE_CYCLE_LENGTH (cycles_t)	((CPU_FREQ/FRAME_FREQ)/2)
#define PAYLOAD_PULSE_CYCLE_LENGTH (cycles_t)	(20000000)/2

struct timespec carrier_ts = {
	.tv_sec = 0,
	/* Here we provision for the jitter on the timer */
	.tv_nsec = PAYLOAD_PULSE_NSEC - JITTER_NSEC_PERIOD,
};

extern int transmitter;
extern volatile int *spinlock;

#ifdef __BUCKET_BASED_DATA__
#define TSC_CYCLE_PER_DATA 		39
#define DATA_PACKET_SIZE 		1000
#define TSC_MAX_DATA_CYCLE		DATA_PACKET_SIZE * TSC_CYCLE_PER_DATA
int data[DATA_PACKET_SIZE];

/*
 * This is the bucket based implementation
 * TODO return PACKET drop
 */
void modulate_data(void)
{
	int x;
	int bucket=0;
	cycles_t t1;

	t1 = get_cycles();
	while(1){
	//TODO fix ME should be % instead....
		bucket = (get_cycles()-t1)/TSC_CYCLE_PER_DATA;
		if(bucket >= DATA_PACKET_SIZE)
			break;
		if(transmitter){
			*spinlock = data[bucket];
		}
		else
			data[bucket] = *spinlock;
	}
	*spinlock = 0;
}

#else /*__BUCKET_BASED_DATA__*/

#define TSC_CYCLE_PER_DATA 		29
#define DATA_PACKET_SIZE 		2000
#define TSC_MAX_DATA_CYCLE		DATA_PACKET_SIZE * TSC_CYCLE_PER_DATA
int data[DATA_PACKET_SIZE];

/*
 * This is the top up based implementation
 */
void modulate_data(void)
{
	int x;
	cycles_t t1;

	t1 = get_cycles();
	for(x=0;x<DATA_PACKET_SIZE;x++){
		if(transmitter){
			*spinlock = data[x];
		}
		else
			data[x] = *spinlock;
		if((get_cycles()-t1)>TSC_MAX_DATA_CYCLE)
			break;
	}
	*spinlock = 0;
//	fprintf(stderr,"%Ld\n",get_cycles() - t1);
}
#endif /*__BUCKET_BASED_DATA__*/

void tx(void)
{
	int x;
	cycles_t t1, t2, phase, delta = 0, lpj;
	
restart:
	/*
	 * TODO relax CPU here
	 */
	while(  ((t2 = get_cycles()) &~0xff) % ((PAYLOAD_PULSE_CYCLE_LENGTH*20) &~0xff) );
	fprintf(stderr, "%Ld %Ld %Ld\n",PAYLOAD_PULSE_CYCLE_LENGTH, PAYLOAD_PULSE_NSEC, t2);

	phase = 0;
	x=0;

	while(1){
		t1 = get_cycles();

#ifdef __TIMER__
		if(clock_nanosleep(CLOCK_MONOTONIC, TIMER_RELTIME, &carrier_ts, NULL))
			DIE("clock_nanosleep");
		
		/*
		 * 'delta' correspond to the amount of cycle taken away by nanosleep()
		 * In other word, this execution context is not burning CPU cycle 
		 * during 'delta' bus cycle
		 *
		 * In the non __TIMER__ case, 'delta' == 0 hence CPU never goes RELAX
		 *
		 * In the __TIMER__ case the timer is configured to fire at
		 *  PAYLOAD_PULSE_NSEC - JITTER_NSEC_PERIOD
		 * then we do LPJ padding up to PAYLOAD_PULSE_NSEC OR equivalent to
		 * 	PAYLOAD_PULSE_CYCLE_LENGTH
		 * 
		 * JITTER_NSEC_PERIOD define our immunity to noise. The cost of immunity is
		 * CPU cycle consumption
		 */
		delta = (get_cycles() - t1)/2;
		if(delta > PAYLOAD_PULSE_CYCLE_LENGTH){
			fprintf(stderr, "CLOCK Synchronization lost! %Lu %Lu\n",PAYLOAD_PULSE_CYCLE_LENGTH, delta);
			goto restart;
		}
#endif
		/*
		 * Here we do LPJ padding.
		 * After this step '(get_cycles() - t1)/2' should be _very_ close to 
		 * 	PAYLOAD_PULSE_CYCLE_LENGTH
		 */
		lpj = (PAYLOAD_PULSE_CYCLE_LENGTH - delta - phase*2 -(t1-t2)/2 );
		if(lpj < 0){
			fprintf(stderr, "LPJ Synchronization lost! %Lu %Lu\n",PAYLOAD_PULSE_CYCLE_LENGTH, delta);
			goto restart;
		}

		calibrated_ldelay(lpj);

//		fprintf(stderr,"%Lu\n",(get_cycles() - t1));

		/*
		 * Here we are monotonic but we can be out of phase.
		 *
		 * The phase shift can be observed by looking at t2 so our goal is to
		 * compensate for the phase shift with respect to t2
		 *
		 * In general phase shift will accumulate over time ( we integrate 
		 * the noise ) but it is generally constant for each iteration i.e.
		 * we are dealing with white noise
		 *
		 * The amount of shift is directly proportionnal to the time we spend 
		 * here i.e. outside the control of LPJ compensation loop.
		 *
		 * The phase compensation has been added to the LPJ compensation 
		 * directly.
		 *
		 * NOTE that ideally we would want to have an phase offset ==0 so that
		 * data modulation could be directly indexed from that value.
		 */
//		fprintf(stderr,"%Lu %Lu %Lu\n",t2 % PAYLOAD_PULSE_CYCLE_LENGTH, phase, delta);

		modulate_data();

		t2 = get_cycles();

		/* 
		 * This is phase compensation; The problem with the convergence is the 
		 * modulo bcos of the non-linear across the period
		 * MOD is linear within the period only
		 * It would be better to converge in the middle of the period 
		 *
		 * THE PROBLEM Is MOD
		 */
		phase = (((PAYLOAD_PULSE_CYCLE_LENGTH/2) - 
			abs( t2 % PAYLOAD_PULSE_CYCLE_LENGTH - PAYLOAD_PULSE_CYCLE_LENGTH/2)) >> 3);

//		phase = phase - 100000;
		//fprintf(stderr,"%Lu %Lu %Lu %Lu\n",t2 % PAYLOAD_PULSE_CYCLE_LENGTH, phase, PAYLOAD_PULSE_CYCLE_LENGTH, t2);

		if(x && !(x%20)){
		//	fprintf(stderr,"%Lu\n",t2 % PAYLOAD_PULSE_CYCLE_LENGTH);

//			fprintf(stderr, "%Lx %Lx\n", t2, phase);
			fprintf(stderr, "%Ld %Ld %Ld %d %d %d %d %d %d %d %d %d %d %d %d\n", t2, 
				phase, t2 % PAYLOAD_PULSE_CYCLE_LENGTH, 
				data[0], data[100], data[200],
				data[300], data[400], data[500],
				data[600], data[700], data[800],
				data[900], data[1000], data[1100]);
		}
		x++;
	}
}

void tx_init(void)
{	
	int c;

	for(c=0; c<DATA_PACKET_SIZE; c++){
		data[c] = c;
	}
	*spinlock = 0;
}

#endif /*__CHARACTERIZATION__*/




