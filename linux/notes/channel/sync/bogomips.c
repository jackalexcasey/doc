/*
 * bogomips.c - Spinloop calibration
 *
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
#include "logging.h"
#include "atomic_64.h"

#define PRINT(fmt, args...)	\
	do{	\
		fprintf(stderr, fmt, ## args); \
} while (0)

# define DIE(format, ...) do {\
	fprintf(stderr, "Fatal %s %d " format,__FILE__, __LINE__, ## __VA_ARGS__);\
	exit(1);\
}while(0)

int sender = 0;
volatile int *spinlock = NULL;
extern void sender_loop(void);
extern void sniffer_loop(void);
char *program	= "";
const char optstring[] = "c:l";
struct option options[] = {
	{ "",	required_argument,	0, 	'j'	},
	{ "",	required_argument,	0, 	'l'	},
	{ 0,	0,	0,	0 }
};

void usage(void)
{
	printf("usage: [-c cpu sets] [-l sender mode] \n");
}

void help(void)
{
	usage();
}

/*
 * This hides the detail about the method used for communication.
 * For modelization we used shared memory variable.
 * For real implementation we use SMT pipeline contention. In that
 * case this function takes care to set the affinity of the HT siblings...
 */
void open_channel(void)
{
	int fd;
	void *ptr;

	if ((fd = shm_open("channel", O_CREAT|O_RDWR,
					S_IRWXU|S_IRWXG|S_IRWXO)) > 0) {
		if (ftruncate(fd, 1024) != 0)
			DIE("could not truncate shared file\n");
	}
	else
		DIE("Open channel");
	
	ptr = mmap(NULL,1024,PROT_READ|PROT_WRITE,MAP_SHARED,fd,0);
	if(ptr == MAP_FAILED)
		DIE("mmap");

	spinlock = ptr; /* spinlock is the first object in the mmap */
	if(sender)
		*spinlock = 0;
	return;
}

void * worker_thread(void *arg)
{
	int cpu;

	cpu = sched_getcpu();
	PRINT("worker_thread start on CPU %d in %s\n",cpu ,sender ? "Sender mode":"Receiver mode");
	if(sender)
		sender_loop();
	else
		sniffer_loop();

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
			case 'l':
				sender = 1;
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
	 * Open the comm channel
	 */
	open_channel();

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

