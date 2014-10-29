/*
 * Copyright (C) 2010 Cisco Systems
 * Author: Etienne Martineau <etmartin@cisco.com>
 *
 * This work is licensed under the terms of the GNU GPL, version 2.
 */

#include "config.h"

extern int transmitter;

/*
 * Suppose 32Kb L1 cache ,8way ,64byte per line
 * ==> 32kb / 64byte == 512 lines
 * ==> 512 lines / 8way == 64 sets
 * ==> 64 sets * 64 byte = 4096 wrap value
 */

#define mb()    asm volatile("mfence":::"memory")
volatile unsigned char dummy;
unsigned char *rx_buf = NULL;

/* 
 * this is flushing one cache line pointed by this addr
 */
# define __force    __attribute__((force))
static inline void clflush(volatile void *__p)
{
	asm volatile("clflush %0" : "+m" (*(volatile char __force *)__p));
}

void open_c(void)
{
	int fd;

	if ((fd = shm_open("channelrx", O_CREAT|O_RDWR,
					S_IRWXU|S_IRWXG|S_IRWXO)) > 0) {
		if (ftruncate(fd, 1024*32) != 0)
			DIE("could not truncate shared file\n");
	}
	else
		DIE("Open channel");
	
	rx_buf = mmap(0x7f0000030000,1024*32,PROT_READ|PROT_WRITE,MAP_SHARED,fd,0);
	if(rx_buf == MAP_FAILED)
		DIE("mmap");
	fprintf(stderr, "rx_buf mmap ptr %p\n",rx_buf);

}

void zap_cache_line(int linenr)
{
	clflush(&rx_buf[64*linenr]);
//	rx_buf[set] = 0xff;
	mb();
}

void load_cache_line(int linenr)
{
	__builtin_prefetch(&rx_buf[64*linenr]);
//	dummy = rx_buf[64*linenr];
	mb();
}

cycles_t measure_cache_line_access_time(int linenr)
{
	cycles_t t1;

	t1 = get_cycles();
	dummy = rx_buf[64*linenr];
	mb();
	return get_cycles() - t1;
}

void pll(void(*fn)(cycles_t))
{
	int x;
	cycles_t array[512];
	
	open_c();

	for(x=0;x<512;x++){
		//This is the encoding part
		if(!(x%2))
			zap_cache_line(x);
		else
			load_cache_line(x);
		//This is the decoding part
		array[x] = measure_cache_line_access_time(x);
	}

	for(x=0;x<512;x++){
		fprintf(stderr,"\t _%Ld_",array[x]);
	}
}

#if 0
void pll(void(*fn)(cycles_t))
{
	int x;
	cycles_t t1, t2, t3, phase, delta, lpj;
	
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
		lpj = (FRAME_PERIOD_IN_CYCLE - delta - phase -((t1-t2)/2) + PHASE_OFFSET/2 );
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
			fprintf(stderr, "PAYLOAD Synchronization lost! %Lu %Lu\n",PAYLOAD_AVAILABLE_CYCLE, t3);
			goto restart;
		}

		if(x && !(x%60))
			fprintf(stderr, "%Ld %Ld/%Ld %Ld\n", t2, t3,PAYLOAD_AVAILABLE_CYCLE, TIMER_JITTER_IN_CYCLE);

		x++;
	}
}
#endif
