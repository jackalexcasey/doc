#ifndef _CLOCK_H_
#define _CLOCK_H_

/*
 * Basic definition
 */
#define USEC_PER_SEC	1000000
#define NSEC_PER_SEC	1000000000
#define TIMER_JITTER_NSEC_PERIOD	100000 	

#define TIMER_RELTIME 0

uint64_t rdtsc(void);
uint64_t rdtsc_mfence(void);
uint64_t rdtsc_lfence(void);
uint64_t rdgtod(void);
uint64_t rdclock(void);

/* We use 64bit values for the times.  */
typedef unsigned long long int hp_timing_t;
typedef unsigned long long cycles_t;  
typedef uint64_t u64;

hp_timing_t get_clockfreq (void);
inline cycles_t get_cycles(void);
void __ldelay(unsigned long loops);
void calibrated_ldelay(unsigned long loops);
void calibrated_timer(unsigned long loops, struct timespec *ts);
void calibrated_stream_tx(int size, int *data);

#endif
