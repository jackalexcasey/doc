#include "config.h"

cycles_t offset = 0;
int ascii = 0;
int pattern = 0;
int playback = 0;
int transmitter = 0;
char *program	= "";
const char optstring[] = "m:c:tapPo:";

struct option options[] = {
	{ "",	required_argument,	0, 	'j'	},
	{ "",	required_argument,	0, 	'l'	},
	{ 0,	0,	0,	0 }
};

void usage(void)
{
	printf("usage: [-c cpu sets] [-t transmitter mode] [-m mmap addr] [-a ascii] [-p playback] [-P pattern] [-o TSC offset]\n");
}

void help(void)
{
	usage();
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
			default:
				ERROR(0, "unknown option '%c'", c);
				++errs;
				break;
		}
	}
#ifdef CACHE_CHANNEL
	cache_open_channel(pci_mem_addr);
#endif
#ifdef SHM_CHANNEL
	shm_open_channel(pci_mem_addr);
#endif

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
	return 0;
}

