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

#ifdef __CHARACTERIZATION__

extern int transmitter;
unsigned long hit = 0;
extern volatile int *spinlock;

int data[1024*1024*20];


/*
 * This loop measure the time it takes to modulate a certain amount of data
 * The curve is linear up to a certain point where the underlying noise of
 * the machine makes it totally random.
 *
 *
 * FIRST STEP:
 * From this characterization we need to extract the TSC cycle per DATA
 * and this is going to define TSC_CYCLE_PER_DATA
 * From characterization:
 * 800	 27420	34.275
 * 900	 30868	34.2977777777778
 * 1000	 34288	34.288
 *   ==>34 TSC per data;
 *
 * the goal is to come with a TSC bucket based data steaming method
 * For that matter we need to manipulate the TSC value we get and
 * find the appropriate bucket of data
 */

#define TSC_CYCLE_PER_DATA 39
void modulate_data(int size)
{
	int x;
	int bucket=0;
	cycles_t t1;

	t1 = get_cycles();
	while(bucket < size){
		bucket = (get_cycles()-t1)/TSC_CYCLE_PER_DATA;
		if(transmitter){
			*spinlock = data[bucket];
		}
		else
			data[bucket] = *spinlock;
		hit = hit + data[bucket];
	}
	*spinlock = 0;
}
#if 1 
/*
 * This is the bucket based implementation
 */
void calibration_modulate_data(int size)
{
	int x;
	int bucket=0;
	cycles_t t1;

	t1 = get_cycles();
	for(x=0;x<size;x++){
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
/* this is the top up based implementation */
void calibration_modulate_data(int size)
{
	int x;
	int bucket=0;
	cycles_t t1;

	t1 = get_cycles();
	for(x=0;x<size;x++){
		if(transmitter){
			*spinlock = data[x];
		}
		else
			data[x] = *spinlock;
		hit = hit + data[x];
		if((get_cycles()-t1) > 888888888888)
			break;
	}
	*spinlock = 0;
}
#endif
void characterization(void)
{
	int x,y,match;
	cycles_t t1, delta, prev;

	for(y=100; y<100000; y = y+100){
		match = 0;
		prev = 0;
		for(x=0;x<1000;x++){
			t1 = get_cycles();
			calibration_modulate_data(y);
	//		modulate_data(y);
			delta = (get_cycles() -t1);
			if( delta == prev )
				match++;
			else{
				prev = delta;
				match = 0;
			}
			if(match >10){
				fprintf(stderr,"%d, %Ld %f\n",y, delta, (float)delta / y);
				break;
			}
			else
				usleep(1);
		}
		if(match == 0){
			fprintf(stderr,"Convergence STOP \n");
			return;
		}
	}
}

void tx(void)
{
	characterization();
}

void tx_init(void)
{
}

#endif /*__CHARACTERIZATION__*/

