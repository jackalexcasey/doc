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
#define __CALIBRATED_TIMER__
//#define __CALIBRATED_JIFFIE__

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

#define FREQ 60
#define PERIOD_CPU_CYCLE	CPU_FREQ/FREQ
#define MONOTONIC_PULSE_CYCLE	PERIOD_CPU_CYCLE/2

void calibrate_lpj(void)
{
	int ret, x;
	cycles_t before,delta;

	x=0;
	while(1){
		before = get_cycles();
		calibrated_ldelay(MONOTONIC_PULSE_CYCLE);
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
 * at every n CPU cycle.
 *
 * We know that timer are subjected to jitter. From above calibration we have measured
 * that typically TIMER_JITTER_NSEC_PERIOD is the max
 * NOTE that TIMER_JITTER_NSEC_PERIOD is our immunity to noise. The cost of
 * a higher TIMER_JITTER_NSEC_PERIOD is a higher CPU usage bcos the timer is 
 * shorter ( provision for longer delay ) and in average the algo needs
 * to compensate with LPJ manually
 */
#ifdef __CALIBRATED_TIMER__
#define CPU_FREQ				2393715000

#define FREQ 60
#define PERIOD_CPU_CYCLE	CPU_FREQ/FREQ
#define MONOTONIC_PULSE_CYCLE	PERIOD_CPU_CYCLE/2

#define SEC_PERIOD	(1/FREQ) %1
#define NSEC_PERIOD (NSEC_PER_SEC/FREQ ) /* 1/FREQ * NSEC_PER_SEC */
#define TIMER_JITTER_NSEC_PERIOD	100000 	

struct timespec carrier_ts = {
	.tv_sec = SEC_PERIOD,
	/* Here we provision for the timer jitter */
	.tv_nsec = NSEC_PERIOD - TIMER_JITTER_NSEC_PERIOD,
};

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

void compensated_timer(void)
{
	cycles_t before,delta;

	while(1){
		before = get_cycles();
		calibrated_timer(MONOTONIC_PULSE_CYCLE, &carrier_ts);
		delta = get_cycles() - before;
		fprintf(stderr," %Lu\n", delta);
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



#if 0
void compensated_timer(void)
{
	int ret, x;
	cycles_t t1, t2, error;

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
		calibrated_ldelay(MONOTONIC_PULSE_CYCLE - delta);
		
		fprintf(stderr," %Lu\n", (get_cycles() - before));
		if(!(x%100))
			fprintf(stderr, ".");
	}
}
#endif

