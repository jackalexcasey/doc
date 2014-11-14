/*
 * Copyright 2008 Google Inc. All Rights Reserved.
 * Author: md@google.com (Michael Davidson)
 *
 * Based on time-warp-test.c, which is:
 * Copyright (C) 2005, Ingo Molnar
 */

#include "config.h"

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
	return ret + offset;
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

