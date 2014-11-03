/*
 * Copyright (C) 2010 Cisco Systems
 * Author: Etienne Martineau <etmartin@cisco.com>
 *
 * This work is licensed under the terms of the GNU GPL, version 2.
 */

#include "config.h"

extern int transmitter;

/*
 * Suppose 32Kb L1 cache ,8way ,64byte per line
 * ==> 32kb / 64byte == 512 lines
 * ==> 512 lines / 8way == 64 sets
 * ==> 64 sets * 64 byte = 4096 wrap value
 *
 * theory of operation:
 * there is 64 cache line in a page  (64*64 = 4096)
 *
 * we could encode a 64bit word directly using 1 page ( 1 cache line per bit )
 * 	for(x=0;x<64;x++){
 * 		load_cache_line(x) / zap_cache_line(x)
 *
 * This kicks the prefetcher so we could add some fuzz within the page
 * 	for(x=0;x<64;x++){
 * 		load_cache_line(no_order[x] / zap_cache_line(no_order[x]
 *
 * this _also_ kick the prefetcher because there is a linear progression
 * after each page
 *
 * So now instead of encoding 64bit directly using 1 page we
 * encode 64 bit in 64 page choosen with fuzz
 * ==> SO with 64 pages we can encode 64 bit 64 times
 */
#define U64_BITS_NR 64 /* number of bits in a u64 */
#define CACHE_LINE_SIZE 64 /* One cache line is 64 bytes */
#define CACHE_LINE_PER_PAGE (4096/CACHE_LINE_SIZE) /* There is 64 cache line per page */
#define QUANTUM_PAGE_NR 64
#define QUANTUM_SIZE (QUANTUM_PAGE_NR * CACHE_LINE_PER_PAGE * CACHE_LINE_SIZE )

#define CACHE_SIZE (1024*1024*12) /* 12 Mb L3 cache */
#define NR_QUANTUM_IN_CACHE (CACHE_SIZE/QUANTUM_SIZE)

#define mb() asm volatile("mfence":::"memory")

static inline void clflush(volatile void *__p)
{
	asm volatile("clflush %0" : "+m" (*(volatile char *)__p));
}

static unsigned char *rx_buf = NULL;
static volatile unsigned char dummy;

static const uint64_t no_order[] = { 46, 10, 41, 61, 11, 13, 37, 12, 48, 59, 0, 54, 30, 7, 57, 58, 17, 16, 25, 35, 62, 15, 2, 26, 21, 39, 50, 32, 23, 36, 18, 43, 47, 45, 24, 20, 27, 29, 60, 55, 28, 3, 1, 8, 22, 53, 42, 56, 33, 19, 34, 5, 49, 31, 51, 40, 6, 38, 52, 63, 4, 14, 44, 9};

static void open_c(void)
{
	int fd;

	if ((fd = shm_open("channelrx", O_CREAT|O_RDWR,
					S_IRWXU|S_IRWXG|S_IRWXO)) > 0) {
		if (ftruncate(fd, CACHE_SIZE) != 0)
			DIE("could not truncate shared file\n");
	}
	else
		DIE("Open channel");

	/*
	 * Cache are taggeg by virtual addr + physical addr
	 * SO here we are mmaping the same physical pages across different process
	 * at the _SAME_ VMA
	 */
	rx_buf = mmap((void*)0x7f0000030000,CACHE_SIZE,PROT_READ|PROT_WRITE,MAP_SHARED,fd,0);
	if(rx_buf == MAP_FAILED)
		DIE("mmap");
	fprintf(stderr, "rx_buf mmap ptr %p\n",rx_buf);

}

static void zap_cache_line(int linenr)
{
	clflush(&rx_buf[CACHE_LINE_SIZE*linenr]);
//	rx_buf[set] = 0xff;
	mb();
}

static void load_cache_line(int linenr)
{
	//__builtin_prefetch(&rx_buf[64*linenr]);
	dummy = rx_buf[CACHE_LINE_SIZE*linenr];
	mb();
}

/*
 * HERE we encode 1 u64 over 64 page
 */
static void encode_u64(int pagenr, int bulknr, uint64_t value)
{
	int x;
	uint64_t tmp;

	tmp = value;
	for(x=0;x<U64_BITS_NR;x++){
		if(!(tmp & 0x1))
			load_cache_line(x*CACHE_LINE_PER_PAGE + pagenr +bulknr*64*CACHE_LINE_PER_PAGE);
		tmp = tmp >> 1;
	}

	tmp = value;
	for(x=0;x<U64_BITS_NR;x++){
		if((tmp & 0x1))
			zap_cache_line(x*CACHE_LINE_PER_PAGE + pagenr +bulknr*64*CACHE_LINE_PER_PAGE);
		tmp = tmp >> 1;
	}
}


/*
 * no_order[] is required here to avoid the prefetcher to kick in 
 * ( which detect an incremental pattern )
 * Below is the cacheline access time; At some point the lines are in memory
 * _304__248__244__248__332__248__248__244__248__244__248__244__80__80__80__80__80__80__80__80__80__80
 */
static uint64_t decode_u64(int pagenr, int bulknr)
{
	int x;
	cycles_t t1;
	uint64_t data;

	data = 0;
	for(x=0;x<U64_BITS_NR;x++){
		t1 = get_cycles();
		load_cache_line(no_order[x]*CACHE_LINE_PER_PAGE + pagenr + bulknr*64*CACHE_LINE_PER_PAGE);
		if(get_cycles()-t1 > 200)
			data = data | (uint64_t)1 << no_order[x];
	}
	return data;
}

/*
 * HERE we decode 64 u64 over 64 page which is the maximum we can do over 64 pages
 */
static void decode_64_u64(int bulknr, uint64_t *value)
{
	int x;
	for(x=0;x<64;x++){
		value[x] = decode_u64(x, bulknr);
	}
}

/*
 * HERE we encode 64 u64 over 64 page which is the maximum we can do over 64 pages
 * 64 pages is 256Kb
 */
static void encode_64_u64(int bulknr,uint64_t *value)
{
	int x;
	for(x=0;x<64;x++){
		encode_u64(x, bulknr, value[x]);
	}
}

void prefetch(void(*fn)(cycles_t))
{
	uint64_t dat64[64],data;
	int x,y;

	cycles_t t1,t2;

	open_c();

#if 0
	t1 = get_cycles();
	load_cache_line(10);
	fprintf(stderr,"%Ld \n",get_cycles()-t1);

	t1 = get_cycles();
	load_cache_line(10);
	fprintf(stderr,"%Ld \n",get_cycles()-t1);

	t1 = get_cycles();
	load_cache_line(10);
	fprintf(stderr,"%Ld \n",get_cycles()-t1);

	zap_cache_line(10);

	t1 = get_cycles();
	load_cache_line(10);
	fprintf(stderr,"%Ld \n",get_cycles()-t1);

	return ;
#endif

	/*
	 * 12mb l3 cache is 48 time (64 pages)
	 * i.e. 48 X uint64_t dat64[64]
	 * 48 is very noisy
	 *
	 * 640X480 requires 4800 uint64_t
	 * ==> 75 X uint64_t dat64[64]
	 */
	fprintf(stderr,"QQ %Ld %Ld\n",QUANTUM_SIZE, NR_QUANTUM_IN_CACHE);
	for(y=0;y<24;y++){
		//Pattern setup
		for(x=0;x<64;x++){
			if(!(x%2))
				dat64[x] = 0x5555555500000000 | y;
			else
				dat64[x] = 0xaaaaaaaa00000000 | y;
		}
		encode_64_u64(y, dat64);
	}

	for(y=0;y<24;y++){
		decode_64_u64(y, dat64);
		for(x=0;x<64;x++){
			if(x==31)
				fprintf(stderr,"%lx ",dat64[x]);
			if(x==32)
				fprintf(stderr,"%lx ",dat64[x]);
		}
		fprintf(stderr,"\n");
	}
	return;
}
