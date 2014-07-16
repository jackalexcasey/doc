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
#define PAYLOAD_PULSE_CYCLE_LENGTH (cycles_t)	(40000000/2)

/* 
 * This is the duty cycle of the PAYLOAD in nsec
 * PAYLOAD_PULSE_NSEC = ((1/FRAME_FREQ) * NSEC_PER_SEC)
 */
#define PAYLOAD_PULSE_NSEC (cycles_t) 			16710427

/*
 * This is the amount of noise we expect on the timer
 * Cannot be greater than PAYLOAD_PULSE_NSEC
 */
//#define JITTER_NSEC_PERIOD (cycles_t)			800000
#define JITTER_NSEC_PERIOD (cycles_t)			12000000

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
void calibrated_ldelay(unsigned long loops)
{
	cycles_t t1, t2, error;
//	loops = loops - LPJ_MAX_RESOLUTION;

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
		 *
		 * The LPJ compensation for the (Jitter on the timer) is 'delta'
		 *
		 * Phase is also compensated to keep t2 inline with PAYLOAD_PULSE_CYCLE_LENGTH
		 */
		lpj = (PAYLOAD_PULSE_CYCLE_LENGTH - delta - phase -((t1-t2)/2) );

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

		calibrated_ldelay(lpj);
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
		 */

		//modulate_data(t2, data);

		phase = ((PAYLOAD_PULSE_CYCLE_LENGTH/2) - 
			abs( (t2 % PAYLOAD_PULSE_CYCLE_LENGTH)/2 - PAYLOAD_PULSE_CYCLE_LENGTH/2) );

		if(x && !(x%60)){
			fprintf(stderr, "%Ld %Ld %Ld %Ld\n", t2, 
				t2 % PAYLOAD_PULSE_CYCLE_LENGTH, lpj, phase);
		}
		x++;
	}
}
