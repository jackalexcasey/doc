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

#include "cpuset.h"
#include "spinlock.h"
#include "threads.h"
#include "logging.h"
#include "clock.h"

char	*program	= "";
const char optstring[] = "c:h";

struct option options[] = {
	{ "cpus",	required_argument,	0, 	'c'	},
	{ "help",	no_argument,		0, 	'h'	},
	{ 0,	0,	0,	0 }
};

void usage(void)
{
	printf("usage: [-h] [-c <cpu_set>]");
}

const char help_text[] =
"check time sources for monotonicity across multiple CPUs\n"
"  -c,--cpus        set of cpus to test (default: all)\n";

void help(void)
{
	usage();
	printf("%s", help_text);
}

void * calibrate_loop(void *arg)
{
	return NULL;
}

int
main(int argc, char *argv[])
{
	int		ncpus;
	int		nthreads;
	int		c;
	cpu_set_t	cpus;
	int		errs;
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
			case 'h':
				help();
				exit(0);
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
	nthreads = create_per_cpu_threads(&cpus, calibrate_loop, NULL);
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

