/*
 * Copyright 2008 Google Inc. All Rights Reserved.
 * Author: md@google.com (Michael Davidson)
 */
#define _GNU_SOURCE

#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <sched.h>
#include <pthread.h>
#include <errno.h>
#include <stdlib.h>
#include <unistd.h>

#include "logging.h"
#include "threads.h"

#define MAX_CPUS CPU_SETSIZE
#define	MAX_THREADS	MAX_CPUS

static thread_t	threads[MAX_THREADS];
static int	num_threads;

#define handle_error_en(en, msg) \
	do { errno = en; perror(msg); exit(EXIT_FAILURE); } while (0)

/*
 * Helper function to run a thread on a specific set of CPUs.
 */
static void *run_thread(void *arg)
{
	thread_t	*thread = arg;
	void		*result;

	if (sched_setaffinity(0, sizeof thread->cpus, &thread->cpus) < 0)
		WARN(errno, "sched_setaffinity() failed");

	result = thread->func(thread->arg);

	return result;
}

static void
display_sched_attr(int policy, struct sched_param *param)
{
   printf("    policy=%s, priority=%d\n",
		   (policy == SCHED_FIFO)  ? "SCHED_FIFO" :
		   (policy == SCHED_RR)    ? "SCHED_RR" :
		   (policy == SCHED_OTHER) ? "SCHED_OTHER" :
		   "???",
		   param->sched_priority);
}

void display_thread_sched_attr(char *msg)
{
   int policy, s;
   struct sched_param param;

   s = pthread_getschedparam(pthread_self(), &policy, &param);
   if (s != 0)
	   handle_error_en(s, "pthread_getschedparam");

   printf("%s\n", msg);
   display_sched_attr(policy, &param);
}




/*
 * Create a set of threads each of which is bound to one of
 * the CPUs specified by cpus.
 * Returns the number of threads created.
 */
int create_per_cpu_threads(cpu_set_t *cpus, thread_func_t func, void *arg)
{
	int	cpu;

	for (cpu = 0; cpu < MAX_CPUS; cpu++) {
		int		err;
		thread_t	*thread;

		if (!CPU_ISSET(cpu, cpus))
			continue;
		if (num_threads >= MAX_THREADS)
			break;

		thread		= &threads[num_threads++];
		thread->func	= func;
		thread->arg	= arg;
		CPU_ZERO(&thread->cpus);
		CPU_SET(cpu, &thread->cpus);

#if 0
//This code doesn't work

		thread->attrp = NULL;

		s= pthread_attr_init(&thread->attr);
		if (s != 0)
			handle_error_en(s, "pthread_attr_init");
		thread->attrp = &thread->attr;

		thread->param.sched_priority = 10;
		
		s = pthread_attr_setschedpolicy(&thread->attr, SCHED_FIFO);
		if (s != 0)
			handle_error_en(s, "pthread_attr_setschedpolicy");

		s = pthread_attr_setschedparam(&thread->attr, &thread->param);
		if (s != 0)
			handle_error_en(s, "pthread_attr_setschedparam");

#endif

		err = pthread_create(&thread->thread, NULL, run_thread, thread);
		if (err) {
			WARN(err, "pthread_create() failed");
			--num_threads;
			break;
		}
#if 0
		s = pthread_attr_destroy(&thread->attr);
		if (s != 0)
			handle_error_en(s, "pthread_attr_destroy");
#endif

	}

	return num_threads;
}


/*
 * Create nthreads threads.
 * Returns the number of threads created.
 */
int create_threads(int nthreads, thread_func_t func, void *arg)
{
	if (nthreads > MAX_THREADS)
		nthreads = MAX_THREADS;

	while (--nthreads >= 0) {
		int		err;
		thread_t	*thread;

		thread		= &threads[num_threads++];
		thread->func	= func;
		thread->arg	= arg;
		CPU_ZERO(&thread->cpus);

		err = pthread_create(&thread->thread, NULL, func, arg);
		if (err) {
			WARN(err, "pthread_create() failed");
			--num_threads;
			break;
		}
	}

	return num_threads;
}


/*
 * Join with the set of previsouly created threads.
 */
void join_threads(void)
{
	while (num_threads > 0)
		pthread_join(threads[--num_threads].thread, NULL);
}

