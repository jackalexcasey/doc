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
//#define PAYLOAD_PULSE_CYCLE_LENGTH ((MONOTONIC_PULSE_CYCLE_LENGTH * PAYLOAD_NSEC_PERIOD) / NSEC_PERIOD)

/* Here we use a rounded value to ease the convergence */
//#define PAYLOAD_PULSE_CYCLE_LENGTH (cycles_t)100000
#define PAYLOAD_PULSE_CYCLE_LENGTH (cycles_t)0x20000

struct timespec carrier_ts = {
	.tv_sec = SEC_PERIOD,
	/* Here we provision for the timer jitter and the payload */
	.tv_nsec = NSEC_PERIOD - JITTER_NSEC_PERIOD - PAYLOAD_NSEC_PERIOD,
};

extern int transmitter;
extern volatile int *spinlock;

/*
 * This function modulate the data over the wire.
 *
 * payload_cycle_length determine the amount of cycle this function is allowed
 * to execute. This value is typically variable because of compensation in the 
 * high level loop.
 *
 * data is transmitted in packet. Packet size is DATA_PACKET_SIZE
 *
 */
#define DATA_PACKET_SIZE 100
int data[DATA_PACKET_SIZE];
unsigned long hit = 0;
void modulate_data(void)
{
	int x;

		for(x=0;x<DATA_PACKET_SIZE;x++){
			if(transmitter){
				*spinlock = data[x];
			}
			else
				data[x] = *spinlock;

			hit = hit + data[x];
		}
		if(transmitter)
			*spinlock = 0;
}

void tx(void)
{
	int x=0, init=0;
	cycles_t t0, t2, delta=0, conv=0;

	/*
	 * t0 mark the start of the cycle. The goal is to keep the system
	 * perfectly monotonic with respect to t0.
	 *
	 * The challenge is that the amount of noise we are subjected to depends
	 * directly on the amount of time spend for DATA transmission
	 *
	 * To achieve this goal we need some compensation. 
	 *  t1 -> t2 == [DATA start, DATA END] + NOISE during [DATA start, DATA END]
	 *  			+ NOISE [t1 to DATA start] + NOISE [ DATA end to t2 ]
	 *  t2 -> t1 == Loop RTT overhead
	 *
	 *  This technique reveal to be not working that well
	 *
	 * OR 
	 *
	 * We compensate from the origin 't0' directly !!!
	 *   =>> t1 = get_cycles(); _not_ needed
	 * This compensation is achieve by computing the delta value
	 *
	 * From that point on we know we are monotonic with respect to T0.
	 *
	 * Now we need to address the phase shift from the two execution
	 * context i.e. we want to get RX and TX in phases. 
	 *
	 * The easiest method is shift back the phase to a common known 
	 * denominator i.e. 0 (assuming same TSC offset from both CPU )
	 *
	 * With this technique we can have the broadcast band aligned so that
	 * data is effectively transferred 
	 *
	 * Without this technique (init = 1 initially ) data cannot be 
	 * transferred OR only very little
	 *
	 */
	fprintf(stderr, "%Lu %Lu\n",PAYLOAD_PULSE_CYCLE_LENGTH, get_cycles());
	
	/* Mark the beginning of time */
	t0 = get_cycles();

	/* Pre-charge t2 to avoid a 'if' within the loop */
	t2 = t0;

	while(1){
	
		/* Here we compensate for the jitter */
		calibrated_ldelay(PAYLOAD_PULSE_CYCLE_LENGTH - delta - conv);

		/* Then in theory we are monotonic right HERE */
		modulate_data();

		/* 
		 * This is how we obtain the retro-feedback compensation value
		 * NOTE that we try to avoid division as much as possible 
		 */
		delta = ((t2 - t0) - (2* x * PAYLOAD_PULSE_CYCLE_LENGTH))>>3;


		if(!(x%0x1000)){
			/* Bring down the offset to 0x50 ; This is frequency tuning */
			conv = conv + ((delta - 0x50 )/4);
//			fprintf(stderr, "%Lx %Lx\n", delta, conv);

#if 1
			/* 
			 * This is how we do phase shift;
			 * First we shift back at coarse level. We've seen that once this
			 * is locked down it typically doesn't move
			 *
			 * //TODO need to improve 
			 */
			if(!init){
//				t0 = t0 - ( ((t2 & 0xfffff)-0x500)  /10);
				if((t2 & 0xf0000) != 0x0)
					t0 = t0 - 0x10000;
				else if((t2 & 0xf000) != 0x0)
					t0 = t0 - 0x1000;
				else
					init = 1;
			}
			fprintf(stderr, "%Lx\n", t2);
#if 0

			if(init){
				fprintf(stderr, "%Lu %Ld %Ld %d %d %d %d %d %d %d %d %d %d %d %d\n", t2, delta, hit,
					data[0], data[1], data[2],
					data[10], data[11], data[12],
					data[20], data[21], data[22],
					data[30], data[31], data[32]);
			}
			else
				fprintf(stderr, "%Lx\n", t2);
#endif

#endif
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





















#if 0
	int x,z;
	unsigned long chunk;
	cycles_t t3, t4, error;

	chunk = payload_cycle_length / DATA_PACKET_SIZE;

	t4 = 0;
	error = 0;
	for(z=0; z<chunk; z++){
		t3 = get_cycles();
		if(!t4)
			t4 = t3;
		error += t3 - t4; /* Measure t4 -> t3 == Loop RTT overhead */

		/* 
		 * Here we cut at the end of the transmission but instead we
		 * want variable bit rate i.e. for every sample we track the
		 * appropriate amount of seek needed.
		 *
		 * OR here we modulate a frame entirely and
		 * repeat it over and over until we reach error >= loops*2
		 */
		for(x=0;x<DATA_PACKET_SIZE;x++){
			if(transmitter)
				*spinlock = data[x];
			else
				data[x] = *spinlock;
		}
		if(transmitter)
			*spinlock = 0;

		t4 = get_cycles();
		error += t4 - t3; /* Measure t3 -> t4 == LPJ delay loop */
		if(error >= payload_cycle_length * 2)
			break;
	}
#endif

#if 0

#if 0
	while(1){
		t1 = get_cycles();
		//__ldelay(LOOP_RESOLUTION);
		__lstream(LOOP_RESOLUTION/10);
		t2 = get_cycles();
		fprintf(stderr, "%Ld\n", t2-t1);
	}
#endif


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

