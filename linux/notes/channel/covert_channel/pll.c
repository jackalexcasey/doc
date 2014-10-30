/*
 * Copyright (C) 2010 Cisco Systems
 * Author: Etienne Martineau <etmartin@cisco.com>
 *
 * This work is licensed under the terms of the GNU GPL, version 2.
 */

#include "config.h"

extern int transmitter;

static struct timespec carrier_ts = {
	.tv_sec = 0,
	.tv_nsec = RELAX_PERIOD_IN_NSEC,
};

void calibrated_ldelay(cycles_t loops)
{
	cycles_t t1;
	t1 = get_cycles();
	while(get_cycles() - t1 < loops );
}

void pll(void(*fn)(cycles_t))
{
	int x;
	cycles_t t1, t2, t3, phase, delta, lpj;

restart:
	/*
	 * Here we adjust the phase on an integer multiple of a frame
	 * TODO relax CPU here
	 */
	while(  ((t2 = get_cycles()) &~0xff) % ((FRAME_PERIOD_IN_CYCLE*FRAME_FREQ*16) &~0xff) );// TODO fix me
	fprintf(stderr, "%Ld %Ld %Ld\n",FRAME_PERIOD_IN_CYCLE, FRAME_PERIOD_IN_NSEC, t2);

	phase = 0;
	x=0;

	while(1){
		t1 = get_cycles();

		if(clock_nanosleep(CLOCK_MONOTONIC, TIMER_RELTIME, &carrier_ts, NULL))
			DIE("clock_nanosleep");
		
		/*
		 * Here 'delta' correspond to the amount of cycle taken away by 
		 * nanosleep(). It cannot exceed the FRAME_PERIOD
		 */
		delta = (get_cycles() - t1)/2;
		if(delta > FRAME_PERIOD_IN_CYCLE){
			fprintf(stderr, "CLOCK Synchronization lost! %Lu %Lu\n",FRAME_PERIOD_IN_CYCLE, delta);
			goto restart;
		}

		/*
		 * Then we do lpj compensation
		 */
		lpj = (FRAME_PERIOD_IN_CYCLE - delta - phase -((t1-t2)/2) + PHASE_OFFSET/2);
		if(lpj < 0){
			fprintf(stderr, ".");
			lpj = 0;
		}

		calibrated_ldelay(lpj*2);

		/*
		 * Phase compensation:
		 *
		 * At t2 we are monotonic but we can be out of phase.
		 *
		 * The (t1-t2) compensation take into account the overall loop 
		 * execution time but even then phase lag can accumulate.
		 *   Example is preemption right after calibrated_ldelay _but_ 
		 *   before t2 = get_cycles();
		 *
		 * In general phase lag will accumulate over time ( i.e we integrate 
		 * the noise ) but it is generally constant for each iteration since
		 * we are dealing with white noise
		 */
		t2 = get_cycles();

		fn(t2);

		phase = ((FRAME_PERIOD_IN_CYCLE/2) - 
			abs( (t2 % FRAME_PERIOD_IN_CYCLE)/2 - FRAME_PERIOD_IN_CYCLE/2) );

		/*
		 * t3 defines the amount of cycle taken by modulation
		 */
		t3 = (get_cycles() - t2)/2;
		if(t3 > PAYLOAD_AVAILABLE_CYCLE){
			fprintf(stderr, "PAYLOAD Synchronization lost! %Ld %Ld/%Ld %Ld/%Ld %Ld @ %d\n", 
				t2, t3,PAYLOAD_AVAILABLE_CYCLE, 
				RELAX_PERIOD_IN_CYCLE, FRAME_PERIOD_IN_CYCLE, lpj, FRAME_FREQ);
			//goto restart;
			//This is not fatal; keep going
		}

		/*
		 * TSC:            A      / B       C        D        E          F
		 * 119553680002228 6024634/30000000 10000000 40000000 23905300 @ 30
		 *
		 * A is time spent in modulation
		 * B is total time available for modulation
		 * C is relax time period
		 * D is total period
		 * E is LPJ compensation ( NOTE E+A == B)
		 * F is frame per second
		 */
		if(x && !(x%FRAME_FREQ))
			fprintf(stderr, "%Ld %Ld/%Ld %Ld/%Ld %Ld @ %d\n", t2, t3,PAYLOAD_AVAILABLE_CYCLE, 
				RELAX_PERIOD_IN_CYCLE, FRAME_PERIOD_IN_CYCLE, lpj, FRAME_FREQ);

		x++;
	}
}

