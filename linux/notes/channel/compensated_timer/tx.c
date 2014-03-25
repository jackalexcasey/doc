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

/* 
 * This is our immunity to noise. What is the largest jitter?
 * The cost of immunity is CPU cycle consumption
 */
#define JITTER_NSEC_PERIOD (cycles_t)			200000

/* 
 * This is the duty cycle of the PAYLOAD in nsec
 * PAYLOAD_PULSE_NSEC = ((1/FRAME_FREQ) * NSEC_PER_SEC)
 */
#define FRAME_FREQ (cycles_t)					9
//#define PAYLOAD_PULSE_NSEC (cycles_t) 			(NSEC_PER_SEC/FRAME_FREQ) 
#define PAYLOAD_PULSE_NSEC (cycles_t) 			(28035444*2*2) 

/* 
 * This is the duty cycle of the PAYLOAD in bus cycle 
 * (CPU_FREQ * (1/FRAME_FREQ))/2
 */
//#define PAYLOAD_PULSE_CYCLE_LENGTH (cycles_t)	((CPU_FREQ/FRAME_FREQ)/2)
#define PAYLOAD_PULSE_CYCLE_LENGTH (cycles_t)	0x8000000

#define VSYNC_PULSE_CYCLE_LENGTH (cycles_t)		0x80000000
#define VSYNC_PULSE_CYCLE_MASK 					0xff000000

struct timespec carrier_ts = {
	.tv_sec = 0,
	/* Here we provision for the timer jitter and the payload */
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
	cycles_t t1, t2, phase, delta = 0, delay;
	
	fprintf(stderr, "%Ld %Ld\n",PAYLOAD_PULSE_CYCLE_LENGTH, PAYLOAD_PULSE_NSEC);

restart:
	/*
	 * First we align the execution context on the same 
	 * time base
	 * TODO relax CPU here
	 *
	 * TODO PROPER SYNC
	 */
	while( ((t2 = get_cycles()) & VSYNC_PULSE_CYCLE_MASK) != 
		(VSYNC_PULSE_CYCLE_LENGTH | PAYLOAD_PULSE_CYCLE_LENGTH));
	
	fprintf(stderr, "%Lx %Lx\n",PAYLOAD_PULSE_CYCLE_LENGTH, get_cycles());

	phase = 0;
	x=0;

	while(1){
		t1 = get_cycles();

#ifdef __TIMER__
		if(clock_nanosleep(CLOCK_MONOTONIC, TIMER_RELTIME, &carrier_ts, NULL))
			DIE("clock_nanosleep");
		delta = (get_cycles() - t1);
#endif
		/*
		 * 'delta' correspond to the amount of cycle taken away by nanosleep()
		 * In other word, this execution context is not burning CPU cycle 
		 * during 'delta' bus cycle
		 *
		 * In the non __TIMER__ case, 'delta' == 0 hence CPU never goes RELAX
		 *
		 * In the __TIMER__ case the delay should be within the JITTER_NSEC_PERIOD
		 */
		delay = PAYLOAD_PULSE_CYCLE_LENGTH - delta/2 - 2*phase - (t1-t2);
		if(delay < 0){
			fprintf(stderr, "Synchronization lost!\n");
			goto restart;
		}
		calibrated_ldelay(delay);
//		fprintf(stderr,"%Lu %Lu\n",delay, get_cycles()-t1);

		/* Then in theory we are monotonic right HERE */
		modulate_data();

		/* 
		 * This is phase compensation;
		 */
		phase = ((PAYLOAD_PULSE_CYCLE_LENGTH/2) - 
			abs( t2 % PAYLOAD_PULSE_CYCLE_LENGTH - PAYLOAD_PULSE_CYCLE_LENGTH/2)) >> 3;

		if(!(x%0x10)){
//			fprintf(stderr, "%Lx %Lx\n", t2, phase);
			fprintf(stderr, "%Lx %Ld %d %d %d %d %d %d %d %d %d %d %d %d\n", t2, phase, 
				data[0], data[100], data[200],
				data[300], data[400], data[500],
				data[600], data[700], data[800],
				data[900], data[1000], data[1100]);
		}

		x++;
		t2 = get_cycles();
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




