
#include "config.h"

#include<stdio.h>
#include <termios.h>
#include <unistd.h>

cycles_t offset = 0;
volatile int page_on = 0;
int ascii = 0;
int pattern = 0;
int playback = 0;
int transmitter = 0;
char *program	= "";
const char optstring[] = "m:c:tapPo:W:";

struct option options[] = {
	{ "",	required_argument,	0, 	'j'	},
	{ "",	required_argument,	0, 	'l'	},
	{ 0,	0,	0,	0 }
};

void usage(void)
{
	printf("usage: [-c cpu sets] [-t transmitter mode] [-m mmap addr (0xdead for ANON)] "
		"[-a ascii] [-p playback] [-P pattern] [-o TSC offset] [-W one page]\n");
}

void help(void)
{
	usage();
}

void * tuning_thread(void *arg)
{
	int c;
	static struct termios oldt, newt;

	/*tcgetattr gets the parameters of the current terminal
	STDIN_FILENO will tell tcgetattr that it should write the settings
	of stdin to oldt*/
	tcgetattr( STDIN_FILENO, &oldt);
	/*now the settings will be copied*/
	newt = oldt;

	/*ICANON normally takes care that one line at a time will be processed
	that means it will return if it sees a "\n" or an EOF or an EOL*/
	newt.c_lflag &= ~(ICANON);

	/*Those new settings will be set to STDIN
	TCSANOW tells tcsetattr to change attributes immediately. */
	tcsetattr( STDIN_FILENO, TCSANOW, &newt);

	/*This is your part:
	I choose 'e' to end input. Notice that EOF is also turned off
	in the non-canonical mode*/
	while((c=getchar())!= 'e'){
		if(c == 'w')
			page_on++;
		else if(c == 'p')
			offset+=FRAME_PERIOD_IN_CYCLE*FRAME_FREQ;
		else if(c == 'o')
			offset-=FRAME_PERIOD_IN_CYCLE*FRAME_FREQ;
	}

	/*restore the old settings*/
	tcsetattr( STDIN_FILENO, TCSANOW, &oldt);

	return 0;
}

void * worker_thread(void *arg)
{
#ifdef CACHE_CHANNEL
	pll(modulate_cache);
#endif
#ifdef SHM_CHANNEL
	pll(modulate_shm);
#endif
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
			case 'P':
				pattern = 1;
				break;
			case 'p':
				playback = 1;
				break;
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
			case 'o':
				offset = strtoul(optarg, NULL, 0);
				break;
			case 'W':
				page_on = strtoul(optarg, NULL, 0);
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

#ifdef CACHE_CHANNEL
	cache_open_channel(pci_mem_addr);
#endif
#ifdef SHM_CHANNEL
	shm_open_channel(pci_mem_addr);
#endif

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

	/*
	 * Create the tuning thread
	 */
	memset(&cpus, 0, sizeof(cpu_set_t));
	CPU_SET(1, &cpus);
	create_per_cpu_threads(&cpus, tuning_thread, NULL);

	join_threads();
	return 0;
}

