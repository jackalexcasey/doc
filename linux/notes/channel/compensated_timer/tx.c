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
#define PAYLOAD_PULSE_NSEC (cycles_t) 			16710427 *4

/*
 * This is the amount of noise we expect on the timer
 */
//#define JITTER_NSEC_PERIOD (cycles_t)			200000
#define JITTER_NSEC_PERIOD (cycles_t)			12000000 *4
//Here the amount of data affect that value SO we may want to specify the
//amount of CPU % then derive everything else from it...

struct timespec carrier_ts = {
	.tv_sec = 0,
	/* Here we provision for the jitter on the timer */
	.tv_nsec = PAYLOAD_PULSE_NSEC - JITTER_NSEC_PERIOD,
};

extern int transmitter;
extern volatile int *spinlock;

extern unsigned char Untitled_bits[];
extern int screen_dump(unsigned char *data);
extern int screen_init(int w, int h);


#ifdef __BUCKET_BASED_DATA__

/* 
 * By increasing the TSC_CYCLE_PER_DATA we increase the immunity to noise
 * BUT we have to crank the JITTER_NSEC_PERIOD to cope with a longer 
 * transmission time
 */
#define TSC_CYCLE_PER_DATA 		39
//#define TSC_CYCLE_PER_DATA 		500
#define PIXEL_WIDTH				640
#define PIXEL_HEIGHT			480
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

int phase_debug[1024*1024];
void dump_phase(void)
{
	int y;
	for(y=0;y<DATA_PACKET_SIZE;y++){
		if(!(y%12))
			fprintf(stderr, "\n");
		fprintf(stderr, "%d ",phase_debug[y]);
	}
}

void dump_data(void)
{
	int y;
	fprintf(stderr,"\n#define Untitled_width 80\n"
		"#define Untitled_height 400\n"
		"unsigned char u_bits[] = {");
	for(y=0;y<DATA_PACKET_SIZE;y++){
		if(!(y%12))
			fprintf(stderr, "\n");
		fprintf(stderr, "0x%2.2x, ",data[y]);
	}
	fprintf(stderr, "};\n");
}

void dump_data_full(void)
{
	int y;
	fprintf(stderr,"\n");
	for(y=0;y<DATA_PACKET_SIZE;y++){
		if(!(y%6))
			fprintf(stderr, "\n");
		fprintf(stderr, "%2d//%2d ",data[y], (unsigned char )y);
	}
	fprintf(stderr, "\n");
}

void tx(void)
{
	int x;
	cycles_t t1, t2, phase, delta = 0, lpj;
	
restart:
	/*
	 * TODO relax CPU here
	 */
	while(  ((t2 = get_cycles()) &~0xff) % ((PAYLOAD_PULSE_CYCLE_LENGTH*60) &~0xff) );
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
		 *
		 */
		delta = (get_cycles() - t1)/2;
		if(delta > PAYLOAD_PULSE_CYCLE_LENGTH){
			fprintf(stderr, "CLOCK Synchronization lost! %Lu %Lu\n",PAYLOAD_PULSE_CYCLE_LENGTH, delta);
			goto restart;
		}
		// TODO here we catch the clock sync but there is other places as well
		// WE probably need a t3
		if((t1 - t2) > 10*PAYLOAD_PULSE_CYCLE_LENGTH){
			fprintf(stderr, "AACLOCK Synchronization lost! %Lu %Lu\n",PAYLOAD_PULSE_CYCLE_LENGTH, delta);
			goto restart;
		}
#endif
		/*
		 * Here we do LPJ padding.
		 * After this step '(get_cycles() - t1)/2' should be _very_ close to 
		 * 	PAYLOAD_PULSE_CYCLE_LENGTH
		 */
		lpj = (PAYLOAD_PULSE_CYCLE_LENGTH - delta - phase -((t1-t2)/2) );

		/*
		 * With proper initial phase alignment this overshoot in phase compensation
		 * should not occurs
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

		modulate_data(t2, data);

		phase = ((PAYLOAD_PULSE_CYCLE_LENGTH/2) - 
			abs( (t2 % PAYLOAD_PULSE_CYCLE_LENGTH)/2 - PAYLOAD_PULSE_CYCLE_LENGTH/2) );
//		phase_debug[x] = phase;
		screen_dump(data);

		if(x && !(x%60)){
			fprintf(stderr, "%Ld %Ld %Ld %d %d %d %d %d %d %d %d %d %d %d %d\n", t2, 
				t2 % PAYLOAD_PULSE_CYCLE_LENGTH, lpj,
				data[0], data[100], data[200],
				data[300], data[400], data[500],
				data[600], data[700], data[800],
				data[900], data[1000], data[1100]);
			if(!transmitter){// && x ==240){
//				dump_data();
//				screen_dump(data);
//				dump_data_full();
//				dump_phase();
//				exit(-1);
			}
		}
		x++;
	}
}

void rx_init(void)
{
	screen_init(PIXEL_WIDTH, PIXEL_HEIGHT);
}

void tx_init(void)
{	
	int c;
	screen_init(PIXEL_WIDTH, PIXEL_HEIGHT);
#if 0
	for(c=0; c<DATA_PACKET_SIZE; c++){
		data[c] = c;
	}
#else
	/* Data source is bitmap */
	memcpy(data,Untitled_bits,DATA_PACKET_SIZE);
	screen_dump(Untitled_bits);
#endif
	*spinlock = 0;
	memset(phase_debug,0,sizeof(phase_debug));
}

#endif /*__CHARACTERIZATION__*/




