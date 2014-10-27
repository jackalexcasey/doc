/*
 * Copyright (C) 2010 Cisco Systems
 * Author: Etienne Martineau <etmartin@cisco.com>
 *
 * This work is licensed under the terms of the GNU GPL, version 2.
 */

#include "config.h"

struct timespec carrier_ts = {
	.tv_sec = 0,
	/* Here we provision for the jitter on the timer */
	.tv_nsec = FRAME_PERIOD_IN_NSEC - TIMER_JITTER_IN_NSEC,
};

void calibrated_ldelay(cycles_t loops)
{
	cycles_t t1, t2, error;
	t1 = get_cycles();
	while(get_cycles() - t1 < loops );
}

void pll(void)
{
	int x;
	cycles_t t1, t2, phase, delta = 0, lpj;
	
restart:
	/*
	 * Here we adjust the phase on an integer multiple of a frame
	 * TODO relax CPU here
	 */
	while(  ((t2 = get_cycles()) &~0xff) % ((FRAME_PERIOD_IN_CYCLE*FRAME_FREQ) &~0xff) );
	fprintf(stderr, "%Ld %Ld %Ld\n",FRAME_PERIOD_IN_CYCLE, FRAME_PERIOD_IN_NSEC, t2);

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
		 *  FRAME_PERIOD_IN_NSEC - TIMER_JITTER_IN_NSEC
		 * then we do LPJ padding up to FRAME_PERIOD_IN_NSEC OR equivalent to
		 * 	FRAME_PERIOD_IN_CYCLE
		 * 
		 * TIMER_JITTER_IN_NSEC define our immunity to noise. The cost of immunity is
		 * CPU cycle consumption
		 *
		 */
		delta = (get_cycles() - t1)/2;
		if(delta > FRAME_PERIOD_IN_CYCLE){
			fprintf(stderr, "CLOCK Synchronization lost! %Lu %Lu\n",FRAME_PERIOD_IN_CYCLE, delta);
			goto restart;
		}

		/*
		 * Here we do LPJ padding.
		 * After this step '(get_cycles() - t1)/2' should be _very_ close to 
		 * 	FRAME_PERIOD_IN_CYCLE
		 *
		 * NOTE:
		 * TIMER_JITTER_IN_NSEC is the immunity to noise which include
		 *  (Jitter on the timer) + (workload + workload ( noise ) + loop)
		 *
		 * The LPJ compensation for the (workload + workload ( noise ) + loop)
		 * is 't1 - t2'
		 *   The work is directly related to TSC_CYCLE_PER_DATA
		 *
		 * The LPJ compensation for the (Jitter on the timer) is 'delta'
		 *
		 * Phase is also compensated to keep t2 inline with FRAME_PERIOD_IN_CYCLE
		 */
		lpj = (FRAME_PERIOD_IN_CYCLE - delta - phase -((t1-t2)/2) + PHASE_OFFSET/2 );

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
//		fprintf(stderr,"%Lu\n",t2 % FRAME_PERIOD_IN_CYCLE);

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
		 * for the timer jitter TIMER_JITTER_IN_NSEC
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

//		modulate_data(t2, data);

		phase = ((FRAME_PERIOD_IN_CYCLE/2) - 
			abs( (t2 % FRAME_PERIOD_IN_CYCLE)/2 - FRAME_PERIOD_IN_CYCLE/2) );

		if(x && !(x%60))
			fprintf(stderr, "%Ld %Ld %Ld\n", t2, t2 % FRAME_PERIOD_IN_CYCLE, lpj);

		x++;
	}
}

