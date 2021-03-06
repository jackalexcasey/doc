#include "config.h"

static volatile int *spinlock = NULL;

/* 
 * By increasing the TSC_CYCLE_PER_DATA we increase the immunity to noise
 * BUT we have to crank the JITTER_NSEC_PERIOD to cope with a longer 
 * transmission time
 */
#define TSC_CYCLE_PER_DATA 		39
#define TSC_MAX_DATA_CYCLE		DATA_PACKET_SIZE * TSC_CYCLE_PER_DATA

/*
 * This is the bucket based implementation
 * TODO return PACKET drop
 */
void modulate_shm(cycles_t init)
{
	int x;
	int bucket=0;
	unsigned char *data;

	data = get_frame_ptr();

	init = init - PHASE_OFFSET;
/*
 * With the bucked based implementation we see a lot of backward
 * ripple
 * EX: 50 52 42 53 58 55 56 58 62 59 60 64
 *
 * With the horizontal line this is almost perfect
 * With the verttical like there is a lot of jitter but the basic shape is here
 */
#if 1
	while(1){
		bucket = (get_cycles()-init)/TSC_CYCLE_PER_DATA;
		if(bucket >= DATA_PACKET_SIZE)
			break;
		if(transmitter){
			*spinlock = data[bucket];
		}
		else
			data[bucket] = *spinlock;
	}
#else
/*
 * with the linear bucket there is no such problem
 * With the vertical line like there is just too much jitter
 *   no basic shape
 * With the horizontal shape its OK but there is a big offset
 */
	for(bucket = (get_cycles()-init)/TSC_CYCLE_PER_DATA; bucket<DATA_PACKET_SIZE; bucket++){
		if(transmitter){
			*spinlock = buf[bucket];
		}
		else
			buf[bucket] = *spinlock;
	}
#endif

	*spinlock = 0;

	dump_frame(data);
}

/*
 * This hides the detail about the method used for communication.
 * For modelization we used shared memory variable.
 * For real implementation we use SMT pipeline contention. In that
 * case this function takes care to set the affinity of the HT siblings...
 */
void shm_open_channel(unsigned long long pci_mem_addr)
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
		*spinlock = 0;
	
	display_init();

	return;
}

