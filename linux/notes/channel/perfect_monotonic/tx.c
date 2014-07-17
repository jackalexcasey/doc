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
extern int ascii;

#define CPU_FREQ								2393715000
#define FRAME_FREQ (cycles_t)					60

/* 
 * This is the duty cycle of the PAYLOAD in bus cycle 
 * CPU_FREQ * (1/FRAME_FREQ)
 * _AND_ must be rounded off to high digit
 * EX: 60 HZ ==> 39895250 cycle
 *  ==> 40000000 rounded
 * ==> 40000000 * 1/CPU_FREQ == 16710427 ~=16msec
 */
#define PAYLOAD_PULSE_CYCLE_LENGTH (cycles_t)	(40000000*4/2)

/* 
 * This is the duty cycle of the PAYLOAD in nsec
 * PAYLOAD_PULSE_NSEC = ((1/FRAME_FREQ) * NSEC_PER_SEC)
 */
#define PAYLOAD_PULSE_NSEC (cycles_t) 			16710427*4

/*
 * This is the amount of noise we expect on the timer
 * _and_ the amount of time spend doing the workload
 * At the end this will define a CPU duty cycle in CPU load
 *
 * Cannot be greater than PAYLOAD_PULSE_NSEC
 */
#define JITTER_NSEC_PERIOD (cycles_t)			12000000*4

/*
 * Phase compensation cannot be negative ( because there is no 
 * negative delay ... ) so phase 0 is defined as PHASE_OFFSET
 */
#define PHASE_OFFSET 2000

struct timespec carrier_ts = {
	.tv_sec = 0,
	/* Here we provision for the jitter on the timer */
	.tv_nsec = PAYLOAD_PULSE_NSEC - JITTER_NSEC_PERIOD,
};

/*
 * LPJ is very accurate and takes exactly 2 bus cycle per loop. It can 
 * actually measure things like the interruption cycle
 * EX for a MONOTONIC_PULSE_CYCLE:
 *  100040, 100040, 112812, 100060, 100040
 *  The typical overhead is 40 CYCLE but sometime where there is IRQ this
 *  number can be much larger.
 *
 * NOTE that interruption cannot be taken away hence if MONOTONIC_PULSE_CYCLE
 * goes to something large it will at some point accumulate many IRQs which
 * will enlarge the value.
 * In other word, LPJ is only accurate for small value most of the time and
 * sometime it goes out of bound. For that reason we need a convergence
 * loop based on a series of small LPJ value and adjust accordingly.
 *
 */
#define LPJ_MAX_RESOLUTION 100
void calibrated_ldelay(cycles_t loops)
{
	cycles_t t1, t2, error;
	t1 = get_cycles();
	while(get_cycles() - t1 < loops );

#if 0
	/* 
	 * Here we compensate for the loop itself.
	 * In order to keep it simple we do the following:
	 *  t1 -> t2 == LPJ delay loop
	 *  t2 -> t1 == Loop RTT overhead
	 */
	t1 = 0;
	t2 = 0;
	error = 0;
	do{
		t1 = get_cycles();
		if(!t2)
			t2 = t1;
		error += t1 - t2; /* Measure t2 -> t1 == Loop RTT overhead */
		__ldelay(LPJ_MAX_RESOLUTION);
		t2 = get_cycles();
		error += t2 - t1; /* Measure t1 -> t2 == LPJ delay loop */
	}while(error < loops*2);
#endif
}

extern int transmitter;
extern volatile int *spinlock;

extern unsigned char Untitled_bits[];
extern int screen_dump(unsigned char *data);
extern int screen_init(int w, int h);

/* 
 * By increasing the TSC_CYCLE_PER_DATA we increase the immunity to noise
 * BUT we have to crank the JITTER_NSEC_PERIOD to cope with a longer 
 * transmission time
 */
#define TSC_CYCLE_PER_DATA 		39

#if 0
#define PIXEL_WIDTH				200
#define PIXEL_HEIGHT			200
#else
#define PIXEL_WIDTH				640
#define PIXEL_HEIGHT			480
#endif

#define DATA_PACKET_SIZE 		(PIXEL_WIDTH*PIXEL_HEIGHT)/8
#define TSC_MAX_DATA_CYCLE		DATA_PACKET_SIZE * TSC_CYCLE_PER_DATA
unsigned char data[DATA_PACKET_SIZE];

/*
 * This is the bucket based implementation
 * TODO return PACKET drop
 */
void modulate_data(cycles_t init, unsigned char *buf)
{
	int x;
	int bucket=0;

	init = init - PHASE_OFFSET;
/*
 * With the bucked based implementation we see a lot of backward
 * ripple
 * EX: 50 52 42 53 58 55 56 58 62 59 60 64
 *
 * With the horizontal line this is almost perfect
 * With the verttical like there is a lot of jitter but the basic shape is here
 */
#if 1
	while(1){
		bucket = (get_cycles()-init)/TSC_CYCLE_PER_DATA;
		if(bucket >= DATA_PACKET_SIZE)
			break;
		if(transmitter){
			*spinlock = buf[bucket];
		}
		else
			buf[bucket] = *spinlock;
	}
#else
/*
 * with the linear bucket there is no such problem
 * With the vertical line like there is just too much jitter
 *   no basic shape
 * With the horizontal shape its OK but there is a big offset
 */
	for(bucket = (get_cycles()-init)/TSC_CYCLE_PER_DATA; bucket<DATA_PACKET_SIZE; bucket++){
		if(transmitter){
			*spinlock = buf[bucket];
		}
		else
			buf[bucket] = *spinlock;
	}
#endif

	*spinlock = 0;
}

void tx(void)
{
	int x;
	cycles_t t1, t2, phase, delta = 0, lpj;
	
restart:
	/*
	 * TODO relax CPU here
	 */
	while(  ((t2 = get_cycles()) &~0xff) % ((PAYLOAD_PULSE_CYCLE_LENGTH*FRAME_FREQ) &~0xff) );
	fprintf(stderr, "%Ld %Ld %Ld\n",PAYLOAD_PULSE_CYCLE_LENGTH, PAYLOAD_PULSE_NSEC, t2);

	phase = 0;
	x=0;

	while(1){
		t1 = get_cycles();

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
		 *
		 */
		delta = (get_cycles() - t1)/2;
		if(delta > PAYLOAD_PULSE_CYCLE_LENGTH){
			fprintf(stderr, "CLOCK Synchronization lost! %Lu %Lu\n",PAYLOAD_PULSE_CYCLE_LENGTH, delta);
			goto restart;
		}

		/*
		 * Here we do LPJ padding.
		 * After this step '(get_cycles() - t1)/2' should be _very_ close to 
		 * 	PAYLOAD_PULSE_CYCLE_LENGTH
		 *
		 * NOTE:
		 * JITTER_NSEC_PERIOD is the immunity to noise which include
		 *  (Jitter on the timer) + (workload + workload ( noise ) + loop)
		 *
		 * The LPJ compensation for the (workload + workload ( noise ) + loop)
		 * is 't1 - t2'
		 *   The work is directly related to TSC_CYCLE_PER_DATA
		 *
		 * The LPJ compensation for the (Jitter on the timer) is 'delta'
		 *
		 * Phase is also compensated to keep t2 inline with PAYLOAD_PULSE_CYCLE_LENGTH
		 */
		lpj = (PAYLOAD_PULSE_CYCLE_LENGTH - delta - phase -((t1-t2)/2) + PHASE_OFFSET/2 );

		/*
		 * With proper initial phase alignment this overshoot in phase compensation
		 * should not occurs
		 * 
		 * This condition means that there is no room for padding the
		 * 	(workload + workload ( noise ) + loop)
		 * 	(Jitter on the timer)
		 * 	(phase)
		 */
		if(lpj < 0){
			fprintf(stderr, ".");
			lpj = 0;
		}

		calibrated_ldelay(lpj*2);
//		fprintf(stderr,"%Lu\n",(get_cycles() - t1));

		t2 = get_cycles();
//		fprintf(stderr,"%Lu\n",t2 % PAYLOAD_PULSE_CYCLE_LENGTH);

		/*
		 * At t2 we are monotonic but we can be out of phase.
		 *
		 * The phase shift can be observed by looking at t2 so our goal is to
		 * compensate for the phase shift with respect to t2
		 *
		 * In general phase shift will accumulate over time ( i.e we integrate 
		 * the noise ) but it is generally constant for each iteration since
		 * we are dealing with white noise
		 *
		 * The amount of shift is directly proportionnal to the time we spend 
		 * here i.e. outside the control of LPJ compensation loop. For that 
		 * reason the DATA_PACKET_SIZE as a direct impact on the room left
		 * for the timer jitter JITTER_NSEC_PERIOD
		 *
		 * The goal is to have a phase phase offset == 0 so that
		 * data modulation could be directly indexed from that value.
		 *
		 * Inter-VM
		 * There is an offset for the TSC when measure across VMs so
		 * for that reason the phase adjustment can't work. TSC read is 
		 * trapped by the hostOS. See README
		 *
		 */

		modulate_data(t2, data);

		phase = ((PAYLOAD_PULSE_CYCLE_LENGTH/2) - 
			abs( (t2 % PAYLOAD_PULSE_CYCLE_LENGTH)/2 - PAYLOAD_PULSE_CYCLE_LENGTH/2) );

		if(ascii){
			if(x && !(x%60)){
				fprintf(stderr, "%Ld %Ld %Ld %d %d %d %d %d %d %d %d %d %d %d %d\n", t2, 
					t2 % PAYLOAD_PULSE_CYCLE_LENGTH, lpj,
					data[0], data[100], data[200],
					data[300], data[400], data[500],
					data[600], data[700], data[800],
					data[900], data[1000], data[1100]);
			}
		}
		else{
			screen_dump(data);
			if(x && !(x%60)){
				fprintf(stderr, "%Ld %Ld %Ld %Ld\n", t2, 
					t2 % PAYLOAD_PULSE_CYCLE_LENGTH, lpj, phase);
			}
		}

		x++;
	}
}


void rx_init(void)
{
	if(!ascii)
		screen_init(PIXEL_WIDTH, PIXEL_HEIGHT);
}

void tx_init(void)
{	
	int c;

	if(ascii){
		for(c=0; c<DATA_PACKET_SIZE; c++){
			data[c] = c;
		}
	}
	else{
		screen_init(PIXEL_WIDTH, PIXEL_HEIGHT);
		/* Data source is bitmap */
		memcpy(data,Untitled_bits,DATA_PACKET_SIZE);
		screen_dump(Untitled_bits);
	}
	*spinlock = 0;
}


