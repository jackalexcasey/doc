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
typedef long long cycles_t;  
typedef uint64_t u64;

inline cycles_t get_cycles(void);

#endif
