/*
 * Copyright 2008 Google Inc. All Rights Reserved.
 * Author: md@google.com (Michael Davidson)
 *
 * Based on time-warp-test.c, which is:
 * Copyright (C) 2005, Ingo Molnar
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
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#include "cpuset.h"
#include "spinlock.h"
#include "threads.h"
#include "logging.h"
#include "clock.h"

#define DECLARE_ARGS(val, low, high)    unsigned low, high
#define EAX_EDX_VAL(val, low, high) ((low) | ((u64)(high) << 32))                                                      
#define EAX_EDX_ARGS(val, low, high)    "a" (low), "d" (high)
#define EAX_EDX_RET(val, low, high) "=a" (low), "=d" (high)
#define rdtscll(val) \
	((val) = __native_read_tsc())             

static __always_inline unsigned long long __native_read_tsc(void)                                                      
{
    DECLARE_ARGS(val, low, high);

    asm volatile("rdtsc" : EAX_EDX_RET(val, low, high));                                                               

    return EAX_EDX_VAL(val, low, high);
}

inline cycles_t get_cycles(void)
{
	unsigned long long ret = 0;
	rdtscll(ret);
	return ret;
}

/* simple loop based delay: Two bus cycle per-loop */
static void delay_loop(unsigned long loops)
{
	asm volatile(
		"	test %0,%0	\n"
		"	jz 3f		\n"
		"	jmp 1f		\n"

		".align 16		\n"
		"1:	jmp 2f		\n"

		".align 16		\n"
		"2:	dec %0		\n"
		"	jnz 2b		\n"
		"3:	dec %0		\n"

		: /* we don't need output */
		:"a" (loops)
	);
}

/*
 * Since we calibrate only once at boot, this
 * function should be set once at boot and not changed
 */
static void (*delay_fn)(unsigned long) = delay_loop;

void __ldelay(unsigned long loops)
{
	delay_fn(loops);
}

/*
 * lstream is just another spinloop but this one is based on 
 * streaming data
 */
extern volatile int *spinlock;
extern int data[1024];
void __lstream(unsigned long loops)
{
	int x,y;
	for(x=0,y=0;x<loops;x++,y++){
		if(y==1024)
			y=0;
		*spinlock = data[y];
	}
}

/*
 * LPJ is very accurate and takes exactly 2 bus cycle per loop. It can 
 * actually measure things like the interruption cycle
 * EX for a MONOTONIC_PULSE_CYCLE:
 *  100040, 100040, 112812, 100060, 100040
 *  The typical overhead is 40 CYCLE but sometime where there is IRQ this
 *  number can be much larger.
 *
 * NOTE that interruption cannot be taken away hence if MONOTONIC_PULSE_CYCLE
 * goes to something large it will at some point accumulate many IRQs which
 * will enlarge the value.
 * In other word, LPJ is only accurate for small value most of the time and
 * sometime it goes out of bound. For that reason we need a convergence
 * loop based on a series of small LPJ value and adjust accordingly.
 *
 */
#define LPJ_MAX_RESOLUTION 100
void calibrated_ldelay(unsigned long loops)
{
	int x;
	unsigned long chunk;
	cycles_t t1, t2, error;

	chunk = loops / LPJ_MAX_RESOLUTION;
//	fprintf(stderr, "%Lu %Lu\n",loops, chunk);

	/* 
	 * Running the loop itself has a noticeable impact when the chunk size
	 * tends toward 0. For that reason we compensate for the loop itself.
	 * In order to keep it simple we do the following:
	 *  t1 -> t2 == LPJ delay loop
	 *  t2 -> t1 == Loop RTT overhead
	 */
	t1 = 0;
	t2 = 0;
	error = 0;
	for(x=0; x<chunk; x++){
		t1 = get_cycles();
		if(!t2)
			t2 = t1;
		error += t1 - t2; /* Measure t2 -> t1 == Loop RTT overhead */
//		__ldelay(LPJ_MAX_RESOLUTION);
		__lstream(LPJ_MAX_RESOLUTION);
		t2 = get_cycles();
		error += t2 - t1; /* Measure t1 -> t2 == LPJ delay loop */
		if(error >= loops*2){ 
//			fprintf(stderr, "%Lu %d %Lu\n",error, x);
			return;
		}
	}
}

/*
 * The goal of calibrated timer is to have 'perfect' monotonic pulse.
 * Timer on their own cannot achieve that goal since they are subject to
 * jitter. The trick here is to use a calibrated loop to 'pad' the jitter out.
 *
 * We know that timer are subjected to jitter. From above calibration we have measured
 * that typically TIMER_JITTER_NSEC_PERIOD is the max
 * NOTE that TIMER_JITTER_NSEC_PERIOD is our immunity to noise. The cost of
 * a higher TIMER_JITTER_NSEC_PERIOD is a higher CPU usage bcos the timer is 
 * shorter ( provision for longer delay ) and in average the algo needs
 * to compensate with LPJ manually
 */
void calibrated_timer(unsigned long loops, struct timespec *ts)
{
	int ret;
	cycles_t t1, delta;

//	fprintf(stderr, "%Lu %Lu\n",ts->tv_sec, ts->tv_nsec);

	t1 = get_cycles();
	ret = clock_nanosleep(CLOCK_MONOTONIC, TIMER_RELTIME, ts, NULL);
	if(ret)
		DIE("clock_nanosleep");
	delta  = (get_cycles() - t1)/2;
	if(delta > loops){
		fprintf(stderr,"#");
		return;
	}
	calibrated_ldelay(loops - delta);
}


/*
 * get the TSC as 64 bit value with CPU clock frequency resolution
 */
#if defined(__x86_64__)
uint64_t rdtsc(void)
{
	uint32_t	tsc_lo, tsc_hi;
	__asm__ __volatile__("rdtsc" : "=a" (tsc_lo), "=d" (tsc_hi));
	return ((uint64_t)tsc_hi << 32) | tsc_lo;
}
#elif defined(__i386__)
uint64_t rdtsc(void)
{
	uint64_t	tsc;
	__asm__ __volatile__("rdtsc" : "=A" (tsc));
	return tsc;
}
#else
#error "rdtsc() not implemented for this architecture"
#endif


uint64_t rdtsc_mfence(void)
{
	__asm__ __volatile__("mfence" ::: "memory");
	return rdtsc();
}


uint64_t rdtsc_lfence(void)
{
	__asm__ __volatile__("lfence" ::: "memory");
	return rdtsc();
}


/*
 * get result from gettimeofday() as a 64 bit value
 * with microsecond resolution
 */
uint64_t rdgtod(void)
{
	struct timeval tv;

	gettimeofday(&tv, NULL);
	return (uint64_t)tv.tv_sec * 1000000 + tv.tv_usec;
}


/*
 * get result from clock_gettime(CLOCK_MONOTONIC) as a 64 bit value
 * with nanosecond resolution
 */
uint64_t rdclock(void)
{
	struct timespec ts;

	clock_gettime(CLOCK_MONOTONIC, &ts);
	return (uint64_t)ts.tv_sec * 1000000000 + ts.tv_nsec;
}

hp_timing_t get_clockfreq (void)
{
  /* We read the information from the /proc filesystem.  It contains at
     least one line like
        cpu MHz         : 497.840237
     or also
        cpu MHz         : 497.841
     We search for this line and convert the number in an integer.  */
  static hp_timing_t result;
  int fd;

  /* If this function was called before, we know the result.  */
  if (result != 0)
    return result;

  fd = open ("/proc/cpuinfo", O_RDONLY);
  if (fd != -1)
    {
      /* XXX AFAIK the /proc filesystem can generate "files" only up
         to a size of 4096 bytes.  */
      char buf[4096*4];
      ssize_t n;

      n = read (fd, buf, sizeof buf);
      if (n > 0)
        {
          char *mhz = strstr(buf,"cpu MHz");//memmem (buf, n, "cpu MHz", 7);

          if (mhz != NULL)
            {
              char *endp = buf + n;
              int seen_decpoint = 0;
              int ndigits = 0;

              /* Search for the beginning of the string.  */
              while (mhz < endp && (*mhz < '0' || *mhz > '9') && *mhz != '\n')
                ++mhz;

              while (mhz < endp && *mhz != '\n')
                {
                  if (*mhz >= '0' && *mhz <= '9')
                    {
                      result *= 10;
                      result += *mhz - '0';
                      if (seen_decpoint)
                        ++ndigits;
                    }
                  else if (*mhz == '.')
                    seen_decpoint = 1;

                  ++mhz;
                }

              /* Compensate for missing digits at the end.  */
              while (ndigits++ < 6)
                result *= 10;
            }
        }

      close (fd);
    }
  return result;
}

