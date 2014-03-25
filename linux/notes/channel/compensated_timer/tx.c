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

/*
 * This is the duty cycle of the PAYLOAD in bus cycle
 */
#define PAYLOAD_PULSE_CYCLE_LENGTH (cycles_t)	0x8000000
#define PAYLOAD_PULSE_CYCLE_DATA_MASK			0x7ffffff
#define PAYLOAD_PULSE_CYCLE_CARRY_OVER 			0x7f00000
#define PAYLOAD_PULSE_CYCLE_MASK 				0xf000000

/*
 * This is the duty cycle of the PAYLOAD in nsec
 * (0x4000000 * 1/CPU_FREQ) * 2
 */
#define CPU_FREQ								2393715000
#define PAYLOAD_PULSE_NSEC (cycles_t) 			28035444 * 2 *2
#define JITTER_NSEC_PERIOD (cycles_t)			200000


#define VSYNC_PULSE_CYCLE_LENGTH (cycles_t)		0x80000000
#define VSYNC_PULSE_CYCLE_MASK 					0xff000000

struct timespec carrier_ts = {
	.tv_sec = 0,
	/* Here we provision for the timer jitter and the payload */
	.tv_nsec = PAYLOAD_PULSE_NSEC - JITTER_NSEC_PERIOD,
};

extern int transmitter;
unsigned long hit = 0;
extern volatile int *spinlock;

#if 0
#define TSC_CYCLE_PER_DATA 		39
#define DATA_PACKET_SIZE 		5000
#define TSC_MAX_DATA_CYCLE		DATA_PACKET_SIZE * TSC_CYCLE_PER_DATA
int data[DATA_PACKET_SIZE];

/*
 * This is the bucket based implementation
 */
void modulate_data(void)
{
	int x;
	int bucket=0;
	cycles_t t1;

	t1 = get_cycles();
	while(bucket < DATA_PACKET_SIZE){
		bucket = (get_cycles()-t1)/TSC_CYCLE_PER_DATA;
		if(transmitter){
			*spinlock = data[bucket];
		}
		else
			data[x] = *spinlock;
		hit = hit + data[bucket];
	}
	*spinlock = 0;
}

#else

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
		hit = hit + data[x];
		if((get_cycles()-t1)>TSC_MAX_DATA_CYCLE)
			break;
	}
	*spinlock = 0;
//	fprintf(stderr,"%Ld\n",get_cycles() - t1);
}

#endif

void tx(void)
{
	int x,y;
	cycles_t t1, t2, phase, delta = 0, delay;

restart:
	/*
	 * First we align the execution context on the same 
	 * time base
	 * TODO relax CPU here
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
		 */
		delay = PAYLOAD_PULSE_CYCLE_LENGTH - delta/2 - 2*phase - (t1-t2);
		if(delay < 0){
			fprintf(stderr, "Synchronization lost!\n");
			goto restart;
		}
		calibrated_ldelay(delay);

		/* Then in theory we are monotonic right HERE */
		modulate_data();

		/* 
		 * This is phase compensation;
		 */
		phase = ((PAYLOAD_PULSE_CYCLE_LENGTH/2) - 
			abs( (t2 & PAYLOAD_PULSE_CYCLE_DATA_MASK)
			% PAYLOAD_PULSE_CYCLE_LENGTH - PAYLOAD_PULSE_CYCLE_LENGTH/2)) >> 3;

		if(!(x%0x10)){
//			fprintf(stderr, "%Lx %Lx\n", t2, phase);
			fprintf(stderr, "%Lx %Ld %Ld %d %d %d %d %d %d %d %d %d %d %d %d\n", t2, phase, hit,
				data[0], data[1], data[2],
				data[10], data[11], data[12],
				data[20], data[21], data[22],
				data[30], data[31], data[32]);

			if( (t2 & PAYLOAD_PULSE_CYCLE_MASK) != PAYLOAD_PULSE_CYCLE_LENGTH){
				fprintf(stderr, "Synchronization lost!\n");
				goto restart;
			}
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

























#if 0
This is the effect of printing with respect to phase compensation
116_84910_84912_63713_42497_26584_15999_9392_5427_3109_1781_1022_610_379_256_193_147_144_138_148_136_117_111_99_96_84_87_85_83_102_108_112_119_136_132_133_153_162_156_142_116_109_95_91_83_101_98_113_107_103_112_115_111_121_125_139_137_137_139_127_135_145_125_108_98_110_109_94_102_111_119_108_123_133_127_111_119_136_139_130_128_131_125_127_125_121_137_147_138_123_115_120_118_121_127_120_116_112_126_132_141_146_127_114_124_109_101_88_108_100_102_106_108_102_105_124_134_126_111_115_122_131_127_138_119_118_106_93_85_102_111_130_138_133_137_118_99_108_105_115_111_124_135_152_146_129_118_130_146_127_106_99_112_118_128_129_135_132_148_153_149_127_127_129_137_123_114_120_125_118_101_110_130_118_114_120_111_109_109_116_127_135_123_109_94_79_86_107_128_115_120_123_110_107_121_130_145_148_145_151_158_156_149_138_118_116_125_141_154_136_137_119_108_112_112_111_122_122_137_124_112_110_106_114_119_136_121_130_135_140_147_138_144_129_137_123_134_118_99_110_112_110_126_143_155_155_147_146_130_113_129_147_136_142_148_137_


		data[x%0x100] = phase;

		if(!(x%0x100)){
			if( (t2 & PAYLOAD_PULSE_CYCLE_MASK) != PAYLOAD_PULSE_CYCLE_LENGTH){
				fprintf(stderr, "Synchronization lost!\n");
				goto restart;
			}
			for(y=0;y <0x100;y++){
				fprintf(stderr, "%d_",data[y]);
			}

		/* 
		 * Equivalent to:
		 * calibrated_ldelay(PAYLOAD_PULSE_CYCLE_LENGTH - JITTER);
		 */
		if(clock_nanosleep(CLOCK_MONOTONIC, TIMER_RELTIME, &carrier_ts, NULL))
			DIE("clock_nanosleep");
		delta = (t1 - t2)/2;
		//fprintf(stderr, "%Lu\n", delta);
		delta += (get_cycles() - t1)/2;
		//fprintf(stderr, "%Lu\n", delta);
		calibrated_ldelay((MONOTONIC_PULSE_CYCLE_LENGTH - PAYLOAD_PULSE_CYCLE_LENGTH) - delta);

#endif















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

