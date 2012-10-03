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

#include "cpuset.h"
#include "spinlock.h"
#include "threads.h"
#include "logging.h"
#include "clock.h"

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

