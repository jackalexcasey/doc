/*
 * Copyright (C) 2010 Cisco Systems
 * Author: Etienne Martineau <etmartin@cisco.com>
 *
 * This work is licensed under the terms of the GNU GPL, version 2.
 */

#ifndef __CONFIG_H__
#define __CONFIG_H__

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
#include "clock.h"

#define CPU_FREQ 2393715000
#define FRAME_FREQ (cycles_t)60

/* 
 * This is the frame period in cycle and nsec:
 *
 *  CPU_FREQ * (1/FRAME_FREQ)
 *  _AND_ must be rounded off to high digit to enable bit masking logic
 *
 * EX: 60 HZ ==> 39895250 cycle
 *  ==> 40000000 rounded _AND_ always divided by 2
 * ==> 40000000 * 1/CPU_FREQ == 16710427 ~=16msec
 */
#define FRAME_PERIOD_IN_CYCLE (cycles_t)(40000000/2)
#define FRAME_PERIOD_IN_NSEC (cycles_t)16710427

/*
 * This is the amount of scheduling jitter we expect on the timer
 * The PLL compensation give us a monotonic pulse right after that period
 * Cannot be greater than FRAME_PERIOD_IN_NSEC !
 */
#define TIMER_JITTER_IN_NSEC (cycles_t)12000000

/*
 * Phase compensation cannot be negative ( because there is no 
 * negative delay ... ) so phase 0 is defined as PHASE_OFFSET
 */
#define PHASE_OFFSET 2000
#endif

