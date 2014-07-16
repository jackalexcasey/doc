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
//#define __CALIBRATED_JIFFIE__
//#define __CALIBRATED_LSTREAM__
//#define __CALIBRATED_TX__
//#define __AUTO_CALIBRATION__
#define __TX__

char *program	= "";
const char optstring[] = "m:c:ta";
int transmitter = 0;
int ascii = 0;
volatile int *spinlock = NULL;

struct option options[] = {
	{ "",	required_argument,	0, 	'j'	},
	{ "",	required_argument,	0, 	'l'	},
	{ 0,	0,	0,	0 }
};

void usage(void)
{
	printf("usage: [-c cpu sets] [-t transmitter mode] [-m mmap addr] [-a ascii]\n");
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
extern void tx_init(void);
extern void rx_init(void);
void open_channel(unsigned long long pci_mem_addr)
{
	int fd;
	void *ptr;

	if(!pci_mem_addr){
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
	}
	else{
		fprintf(stderr,"Using provide address cookie 0x%llx\n",pci_mem_addr);
		fd = open ( "/dev/mem", O_RDWR);
		ptr  = mmap(NULL, 1024, PROT_READ|PROT_WRITE, MAP_SHARED, fd, (off_t)pci_mem_addr);
		if (!ptr) {
			printf("Cannot map 0x%llx with size of %x\n",  pci_mem_addr, 1024);
			exit(0);
		}

	}
	spinlock = ptr; /* spinlock is the first object in the mmap */
	if(transmitter)
		tx_init();
	else
		rx_init();
	return;
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

#ifdef __CALIBRATED_LSTREAM__
#define CPU_FREQ				2393715000

#define FREQ 60
#define PERIOD_CPU_CYCLE	CPU_FREQ/FREQ
#define MONOTONIC_PULSE_CYCLE	PERIOD_CPU_CYCLE/2

void calibrate_lstream(void)
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
#endif /* __CALIBRATED_LSTREAM__ */

/*
 * -------------------------------------------------
 *           Compensated timer
 * -------------------------------------------------
 */
#ifdef __CALIBRATED_TIMER__
#define CPU_FREQ				2393715000

#define FREQ 60
#define PERIOD_CPU_CYCLE	CPU_FREQ/FREQ
#define MONOTONIC_PULSE_CYCLE	PERIOD_CPU_CYCLE/2

#define SEC_PERIOD	(1/FREQ) %1
#define NSEC_PERIOD (NSEC_PER_SEC/FREQ ) /* 1/FREQ * NSEC_PER_SEC */

struct timespec carrier_ts = {
	.tv_sec = SEC_PERIOD,
	/* Here we provision for the timer jitter */
	.tv_nsec = NSEC_PERIOD - TIMER_JITTER_NSEC_PERIOD,
};

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

#ifdef __CALIBRATED_TX__
#define CPU_FREQ				2393715000

#define FREQ 60
#define PERIOD_CPU_CYCLE	CPU_FREQ/FREQ
#define MONOTONIC_PULSE_CYCLE	PERIOD_CPU_CYCLE/2

#define SEC_PERIOD	(1/FREQ) %1
#define NSEC_PERIOD (NSEC_PER_SEC/FREQ ) /* 1/FREQ * NSEC_PER_SEC */

struct timespec carrier_ts = {
	.tv_sec = SEC_PERIOD,
	/* Here we provision for the timer jitter */
	.tv_nsec = NSEC_PERIOD - TIMER_JITTER_NSEC_PERIOD,
};


void calibrated_tx(void)
{
	int ret;
	cycles_t t1, t2, t3, delta;

	while(1){
		/* Here we mark the beginning of the cycle */
		t1 = get_cycles();
		fprintf(stderr,"%Lu\n", t1-t3);
		
		/* 
		 * Then we sleep for a period of time define as:
		 *  [(NSEC_PERIOD - TIMER_JITTER) - ((NSEC_PERIOD - TIMER_JITTER) + TIMER_JITTER)]
		 */
		ret = clock_nanosleep(CLOCK_MONOTONIC, TIMER_RELTIME, &carrier_ts, NULL);
		if(ret)
			DIE("clock_nanosleep");

		/* Calculater the real length of the sleep */
		delta = (get_cycles() - t1)/2;

		/* 
		 * If the sleep period is larger than the fundamental period (the timmer
		 * jitter is larger than TIMER_JITTER ) then error out
		 * TODO We need to ripple that value to the next sample
		 */
		if(delta > MONOTONIC_PULSE_CYCLE){
			fprintf(stderr,"#");
			return;
		}
		/*
		 * Then we trigger the padding LPJ to reach the exact value of 
		 * MONOTONIC_PULSE_CYCLE
		 */
		calibrated_ldelay(MONOTONIC_PULSE_CYCLE - delta);
		
		/* 
		 * Then we mark the time after the whole cycle
		 * t2 - t1 should be very close to MONOTONIC_PULSE_CYCLE
		 */
		t2 = get_cycles();

		/*
		 * Then we start TX ops
		 * REMEMBER that we can be interrupted at any point in time 
		 * so the fundamental TX algo must be time adjusted as well
		 */
		calibrated_stream_tx(128, data);
		//WE need to prob back that jitter to the top of the loop
		t3 = get_cycles();
		fprintf(stderr,"%Lu\n", t3-t1);
	}
}
#endif /* __CALIBRATED_TX__ */

#ifdef __AUTO_CALIBRATION__
/*
 * From code POV __ldelay() is very accurate i.e. it takes exactly 2 bus cycle
 * per loop with a typical 'pre-charge' overhead of 40 cycle.
 *
 * From system POV __ldelay() is subject to noise and for that reason the 
 * choice of 'lpj_res' is critical. In other word, the best we can do is to
 * chose 'lpj_res' so that __ldelay() is accurate _most_ of the time. EX:
 * 	If 'lpj_res' is larger than the noise ( timer IRQ period ) then __ldelay()
 * 	will have considerable jitter since A) the delay may contain more than
 * 	one IRQ and B) the IRQs execution time is variable.
 * 
 * Another factor to consider is that the overall construct must stays 
 * MONOTONIC. The internal mechanism to achieve this is to drop packet i.e. 
 * skip some calls to __ldelay().
 *
 * Our goal here is to minimize the jitter and maximize the packet rates
 *
 * lpj_res 13298416, MAX jitter 21416, MAX packet drop 100 % 
 * lpj_res 6649208, MAX jitter 6958, MAX packet drop 50 % 
 * lpj_res 3324604, MAX jitter 17966, MAX packet drop 25 % 
 * lpj_res 1662302, MAX jitter 59378, MAX packet drop 12 % 
 * lpj_res 831151, MAX jitter 18612, MAX packet drop 6 % 
 * lpj_res 415575, MAX jitter 9980, MAX packet drop 3 % 
 * lpj_res 207787, MAX jitter 9898, MAX packet drop 1 % 
 * lpj_res 103893, MAX jitter 33990, MAX packet drop 0 % 
 * lpj_res 51946, MAX jitter 18298, MAX packet drop 0 % 
 * lpj_res 25973, MAX jitter 24998, MAX packet drop 0 % 
 * lpj_res 12986, MAX jitter 12874, MAX packet drop 0 % 
 * lpj_res 6493, MAX jitter 5982, MAX packet drop 0 %  <<< Sweet spot around HERE
 * lpj_res 3246, MAX jitter 3304, MAX packet drop 1 %  <<< Sweet spot around HERE
 * lpj_res 1623, MAX jitter 1708, MAX packet drop 2 %  <<< From HERE
 * lpj_res 811, MAX jitter 844, MAX packet drop 4 %    <<< TO HERE there is a 2X
 * lpj_res 405, MAX jitter 466, MAX packet drop 8 %    <<< SAME
 * lpj_res 202, MAX jitter 232, MAX packet drop 15 %   <<< SAME
 * lpj_res 101, MAX jitter 178, MAX packet drop 26 % 
 */
#define MAX(a,b) (((a)>(b))?(a):(b))
#define MIN(a,b) (((a)<(b))?(a):(b))
void auto_calibration(unsigned long loops, int slow)
{
	int x, y=0,  packet_drop_max = 0, lpj_res = loops;
	unsigned long chunk;
	cycles_t t1, t2, error;
	cycles_t start, jitter, max=0;

	if(slow)
		lpj_res = 6000;

again:
	chunk = loops / lpj_res;
//	fprintf(stderr, "%Lu %Lu\n",loops, chunk);

	start = get_cycles();

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
		__ldelay(lpj_res);
		t2 = get_cycles();
		error += t2 - t1; /* Measure t1 -> t2 == LPJ delay loop */

		/* 
		 * As soon as we exceed the total period we stop and drop
		 * the remainin packet on the floor
		 */
		if(error >= loops*2) 
			break;
	}
	jitter = (get_cycles() - start)/2 - loops;
	max = MAX(jitter, max);
	packet_drop_max = MAX(packet_drop_max, chunk - x);

	y++;
	if(!(y%10)){
		fprintf(stderr,"lpj_res %ld, MAX jitter %Lu, packet NR %d MAX packet drop %d \% \n",
			lpj_res, max, chunk, (packet_drop_max*100)/chunk);
		if(max > 200){
			/* Determine the convergence rate */
			if(slow)
				lpj_res = lpj_res - 100;
			else
				lpj_res = lpj_res / 2;
			if(lpj_res < 10){
				fprintf(stderr,"Cannot converge\n");
				return;
			}
			max = 0;
			packet_drop_max = 0;
			goto again;
		}
		else
			return;
	}
	goto again;
}

void auto_calibration_main(unsigned long cpu_freq)
{
	int x;
	unsigned long freq;
	unsigned long period_cpu_cycle;
	unsigned long monotonic_pulse_cycle;

	for(x=49;x<100;x+=10){
		freq = x+1;
		period_cpu_cycle = cpu_freq / freq;
		monotonic_pulse_cycle = period_cpu_cycle/2;
	
		fprintf(stderr, "auto_calibration Freq %dHZ \n",freq);
		auto_calibration(monotonic_pulse_cycle, 0);
//		auto_calibration(monotonic_pulse_cycle, 1);
	}
}
#endif /* __AUTO_CALIBRATION__ */


void * worker_thread(void *arg)
{
	int cpu, v1;

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
#ifdef __CALIBRATED_LSTREAM__
	calibrate_lstream();
#endif
#ifdef __CALIBRATED_TX__
	if(transmitter)
		calibrated_tx();
	else{
		while(1){
			v1 = *spinlock;
			while(*spinlock == v1);
			fprintf(stderr, "%d\n",v1);
		}
	}
#endif
#ifdef __AUTO_CALIBRATION__
	auto_calibration_main(2393715000);
#endif
#ifdef __TX__
	tx();
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
	unsigned long long pci_mem_addr = 0;
	
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
			case 'a':
				ascii = 1;
				break;
			case 't':
				transmitter = 1;
				break;
			case 'c':
				if (parse_cpu_set(optarg, &cpus) != 0)
					++errs;
				break;
			case 'm':
				pci_mem_addr = strtoul(optarg, NULL, 16);
				break;
			default:
				ERROR(0, "unknown option '%c'", c);
				++errs;
				break;
		}
	}

	open_channel(pci_mem_addr);

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

