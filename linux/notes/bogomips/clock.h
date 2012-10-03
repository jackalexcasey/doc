#ifndef _CLOCK_H_
#define _CLOCK_H_

uint64_t rdtsc(void);
uint64_t rdtsc_mfence(void);
uint64_t rdtsc_lfence(void);
uint64_t rdgtod(void);
uint64_t rdclock(void);

/* We use 64bit values for the times.  */
typedef unsigned long long int hp_timing_t;
hp_timing_t get_clockfreq (void);

#endif
