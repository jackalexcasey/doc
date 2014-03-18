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

//#define __CALIBRATION__
//#define __CALIBRATED_TIMER__
#define __CALIBRATED_JIFFIE__

char *program	= "";
const char optstring[] = "c:";
struct option options[] = {
	{ "",	required_argument,	0, 	'j'	},
	{ "",	required_argument,	0, 	'l'	},
	{ 0,	0,	0,	0 }
};

void usage(void)
{
	printf("usage: [-c cpu sets]\n");
}

void help(void)
{
	usage();
}

/*
 * -------------------------------------------------
 *           Calibration
 * -------------------------------------------------
 */
#ifdef __CALIBRATION__
//#define V_SYNC_SEC_PERIOD	16666666 / NSEC_PER_SEC
//#define V_SYNC_NSEC_PERIOD	16666666 % NSEC_PER_SEC

#define V_SYNC_SEC_PERIOD	41776067 / NSEC_PER_SEC
#define V_SYNC_NSEC_PERIOD	41776067 % NSEC_PER_SEC

const struct timespec carrier_ts = {
	.tv_sec = V_SYNC_SEC_PERIOD,
	.tv_nsec = V_SYNC_NSEC_PERIOD,
};

/*
 * This function measure the jitter ( in TSC cycle ) of the monotonic
 * timer.
 *
 * #define V_SYNC_SEC_PERIOD	16666666 / NSEC_PER_SEC
 * #define V_SYNC_NSEC_PERIOD	16666666 % NSEC_PER_SEC
 * The timer period is 16.66666 msec
 *  For a CPU running at 2393.715 Mhz this is 39895248 CPU cycle.
 *  CPU cycle == TSC cycle
 * 
 * The measurement gives us: 40035032, 40031040, 40027208, 40039688
 *  MIN = 40027208 ==> 131960 jitter cycle OR 55 nsec jitter
 *  MAX = 40039688 ==> 60 nsec
 * 
 * #define V_SYNC_SEC_PERIOD	41776067 / NSEC_PER_SEC
 * #define V_SYNC_NSEC_PERIOD	41776067 % NSEC_PER_SEC
 * The timer period is choosen so that its 100000000 CPU cycle for a CPU at 2393.715 Mhz
 * The measurement gives us: 100145248, 100139712, 100130796
 * ==> Jitter: 145248, 139712, 130796
 */
void calibrate_compensated_timer(void)
{	
	int ret, x;
	cycles_t before,delta;

	x=0;
	while(1){
		before = get_cycles();
		ret = clock_nanosleep(CLOCK_MONOTONIC, TIMER_RELTIME, &carrier_ts, NULL);
		if(ret)
			DIE("clock_nanosleep");
		fprintf(stderr," %Lu\n", (get_cycles() - before));
		if(!(x%V_SYNC_HZ))
			fprintf(stderr, ".");
	}
}
#endif /* __CALIBRATION__ */

/*
 * -------------------------------------------------
 *           Calibration LPJ
 * -------------------------------------------------
 */
#ifdef __CALIBRATED_JIFFIE__
#define CPU_FREQ				2393715000
#define CPU_CYCLE_PER_NSEC		2393
#define MONOTONIC_PULSE_CYCLE	100000000

/*
 * The idea here is to do a SW loop for a certain number of time
 * and see the basis overhead. The LPJ takes exactly 2 cycle per loop
 *
 * LPJ is very accurate. It can actually measure the interruption cycle
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
		__ldelay(LPJ_MAX_RESOLUTION);
		t2 = get_cycles();
		error += t2 - t1; /* Measure t1 -> t2 == LPJ delay loop */
		if(error >= loops*2){ 
//			fprintf(stderr, "%Lu %d %Lu\n",error, x);
			return;
		}
	}
}

void calibrate_lpj(void)
{
	int ret, x;
	cycles_t before,delta;

	x=0;
	while(1){
		before = get_cycles();
		calibrated_ldelay(10000000);
		delta = get_cycles() - before;
		fprintf(stderr," %Lu\n", delta);
		if(!(x%100))
			fprintf(stderr, ".");
	}
}

#endif /*__CALIBRATED_JIFFIE__ */

/*
 * -------------------------------------------------
 *           Compensated timer
 * -------------------------------------------------
 */

/*
 * The goal of compensated timer is to have 'perfect' monotonic pulse.
 * Timer on their own cannot achieve that goal since they are subject to
 * jitter. The trick here is to use a calibrated loop to 'pad' the jitter out.
 *
 * Below the MONOTONIC_PULSE_CYCLE is the end goal i.e. have a perfect pulse
 * at every 100000000 CPU cycle.
 *
 * We know that timer are subjected to jitter. From above calibration we have measured
 * that typically TIMER_MAX_JITTER_CYCLE is the max
 *
 * We calculate the timer period like that
 *  (MONOTONIC_PULSE_CYCLE - TIMER_MAX_JITTER_CYCLE) * 1/CPU_FREQ
 *
 */
#ifdef __CALIBRATED_TIMER__
#define CPU_FREQ				2393715000
#define CPU_CYCLE_PER_NSEC		2393
#define MONOTONIC_PULSE_CYCLE	100000000
#define TIMER_MAX_JITTER_CYCLE	150000 	
#define V_SYNC_SEC_PERIOD	41713403 / NSEC_PER_SEC
#define V_SYNC_NSEC_PERIOD	41713403 % NSEC_PER_SEC

struct timespec carrier_ts = {
	.tv_sec = V_SYNC_SEC_PERIOD,
	.tv_nsec = V_SYNC_NSEC_PERIOD,
};

void compensated_timer(void)
{
	int ret, x;
	cycles_t before, delta, jitter;

	x=0;
	while(1){
		before = get_cycles();
		ret = clock_nanosleep(CLOCK_MONOTONIC, TIMER_RELTIME, &carrier_ts, NULL);
		if(ret)
			DIE("clock_nanosleep");
		delta = get_cycles() - before;

again:
		/* 
		 * If we are pass our max tolerance there is nothing we can do
		 * other than compensating the time base backward
		 */
		if(delta > MONOTONIC_PULSE_CYCLE){
			carrier_ts.tv_nsec = carrier_ts.tv_nsec - 
				((delta - MONOTONIC_PULSE_CYCLE)/CPU_CYCLE_PER_NSEC);
			fprintf(stderr, "#%Lu\n",carrier_ts.tv_nsec);

			/* 
			 * Here this is a cut & paste from above to avoid clobbering the
			 * main loop with if / else
			 */
			before = get_cycles();
			ret = clock_nanosleep(CLOCK_MONOTONIC, TIMER_RELTIME, &carrier_ts, NULL);
			/* Reset the time base to original value */
			carrier_ts.tv_nsec = V_SYNC_NSEC_PERIOD;
			if(ret)
				DIE("clock_nanosleep");
			delta = get_cycles() - before;
			goto again;
		}
		__ldelay(MONOTONIC_PULSE_CYCLE - delta);
		
		fprintf(stderr," %Lu\n", (get_cycles() - before));
		if(!(x%100))
			fprintf(stderr, ".");
	}
}
#endif /* __CALIBRATED_TIMER__ */
void * worker_thread(void *arg)
{
	int cpu;

	cpu = sched_getcpu();
	PRINT("worker_thread start on CPU %d\n",cpu);
#ifdef __CALIBRATION__
	calibrate_compensated_timer();
#endif
#ifdef __CALIBRATED_TIMER__
	compensated_timer();
#endif
#ifdef __CALIBRATED_JIFFIE__
	calibrate_lpj();
#endif

	return NULL;
}

int
main(int argc, char *argv[])
{
	int	c;
	int errs;
	int	ncpus;
	int	nthreads;
	cpu_set_t	cpus;
	extern int	opterr;
	extern int	optind;
	extern char	*optarg;
	
	if ((program = strrchr(argv[0], '/')) != NULL)
		++program;
	else
		program = argv[0];
	set_program_name(program);

	/*
	 * default to checking all cpus
	 */
	for (c = 0; c < CPU_SETSIZE; c++) {
		CPU_SET(c, &cpus);
	}

	opterr = 0;
	errs = 0;

	while ((c = getopt_long(argc, argv, optstring, options, NULL)) != EOF) {
		switch (c) {
			case 'c':
				if (parse_cpu_set(optarg, &cpus) != 0)
					++errs;
				break;
			default:
				ERROR(0, "unknown option '%c'", c);
				++errs;
				break;
		}
	}

	if (errs) {
		usage();
		exit(1);
	}

	/*
	 * limit the set of CPUs to the ones that are currently available
	 * (Note that on some kernel versions sched_setaffinity() will fail
	 * if you specify CPUs that are not currently online so we ignore
	 * the return value and hope for the best)
	 */
	sched_setaffinity(0, sizeof cpus, &cpus);
	if (sched_getaffinity(0, sizeof cpus, &cpus) < 0) {
		ERROR(errno, "sched_getaffinity() failed");
		exit(1);
	}

	/*
 	 * create the threads
 	 */
	ncpus = count_cpus(&cpus);
	nthreads = create_per_cpu_threads(&cpus, worker_thread, NULL);
	if (nthreads != ncpus) {
		ERROR(0, "failed to create threads: expected %d, got %d",
			ncpus, nthreads);
		if (nthreads) {
			join_threads();
		}
		return 1;
	}
	join_threads();
}

