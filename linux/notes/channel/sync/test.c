/*
 * Author: Etienne Martineau <etmartin@cisco.com>
 *
 * This work is licensed under the terms of the GNU GPL, version 2.
 *
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
#include "logging.h"
#include "atomic_64.h"

#define TIMER_RELTIME 0

#define PRINT(fmt, args...)	\
	do{	\
		fprintf(stderr, fmt, ## args); \
} while (0)

# define DIE(format, ...) do {\
	fprintf(stderr, "Fatal %s %d " format,__FILE__, __LINE__, ## __VA_ARGS__);\
	exit(1);\
}while(0)

extern int transmitter;
extern volatile int *spinlock;

typedef unsigned long long cycles_t;  
typedef uint64_t u64;

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

static inline cycles_t get_cycles(void)
{
	unsigned long long ret = 0;
	rdtscll(ret);
	return ret;
}

/*
 * Basic definition
 */
#define USEC_PER_SEC	1000000
#define NSEC_PER_SEC	1000000000

/* V sync is 60HZ => 16666666 nsec period*/
#define V_SYNC_HZ			60
#define V_SYNC_SEC_PERIOD	16666666 / NSEC_PER_SEC
#define V_SYNC_NSEC_PERIOD	16666666 % NSEC_PER_SEC
#define TIMER_AVG_JITTER	100*1000 /* 100 uSec in average */

const struct timespec carrier_ts = {
	.tv_sec = V_SYNC_SEC_PERIOD,
	.tv_nsec = V_SYNC_NSEC_PERIOD,
};

const struct timespec carrier_adj_ts = {
	.tv_sec = V_SYNC_SEC_PERIOD,
	.tv_nsec = 16000000,
};

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

static void __ldelay(unsigned long loops)
{
	delay_fn(loops);
}

void do_carrier(void)
{
	*spinlock = 1;
	__ldelay(10);
	*spinlock = 0;
}


//cat  /proc/22713/sched |grep vrun
//[26268.440857] CPU 0 : lpj 23940150
//[26268.440859] CPU 1 : lpj 23939656
//[26268.440861] CPU 2 : lpj 23939639
//[26268.440862] CPU 3 : lpj 23939644
//[26268.440864] CPU 4 : lpj 23939600
//# The CPU frequency cycle is 2393.711 MHz,
//378 CONFIG_HZ_100=y
//40036728
//
//
void relax_cpu(const struct timespec *request)
{
	int ret;
	static int x=0;
	cycles_t before, delta;

	/*
	 * Here we are compensating the timers using TSC by padding the tail-end 
	 * The avg value is 40040764
	 * so we are padding up to 40200000 
	 */
	before = get_cycles();
	ret = clock_nanosleep(CLOCK_MONOTONIC, TIMER_RELTIME, request, NULL);
	if(ret)
		DIE("clock_nanosleep");
	//40040764 
	delta = (get_cycles() - before);
	if(delta > 40200000){
		fprintf(stderr,"ERROR delta %Lu\n",delta);
		DIE("");
	}
	__ldelay(40200000 - delta);
	fprintf(stderr," %Lu\n", (get_cycles() - before));
	if(!(x%V_SYNC_HZ))
		fprintf(stderr, ".");
	x++;
}

void calibrate(void)
{
	int ret;
	static int x=0;
	cycles_t before, delta;

	while(1){
	before = get_cycles();
	ret = clock_nanosleep(CLOCK_MONOTONIC, TIMER_RELTIME, &carrier_ts, NULL);
	if(ret)
		DIE("clock_nanosleep");
	//__ldelay(40034200/2); // ==> 40479296 // Two cycle per loop
	delta = (get_cycles() - before);
	fprintf(stderr," %Lu\n", delta);
	}

}

void transmitter_loop(void)
{
	calibrate();

	while(1){
		do_carrier();
		relax_cpu(&carrier_ts);
	}
}


void detect_carrier(void)
{
	while(*spinlock == 0);
	while(*spinlock == 1);
}

void receiver_loop(void)
{
	detect_carrier();

	while(1){
		detect_carrier();
		relax_cpu(&carrier_adj_ts);
	}
}


