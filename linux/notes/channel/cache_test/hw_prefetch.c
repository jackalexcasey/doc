/*
 * Copyright (C) 2010 Cisco Systems
 * Author: Etienne Martineau <etmartin@cisco.com>
 *
 * This work is licensed under the terms of the GNU GPL, version 2.
 */

#include "config.h"

extern int transmitter;

#define mb() asm volatile("mfence":::"memory")

# define __force __attribute__((force))
static inline void clflush(volatile void *__p)
{
	asm volatile("clflush %0" : "+m" (*(volatile char __force *)__p));
}

static unsigned char *rx_buf = NULL;
static volatile unsigned char dummy;

/*
 * Suppose 32Kb L1 cache ,8way ,64byte per line
 * ==> 32kb / 64byte == 512 lines
 * ==> 512 lines / 8way == 64 sets
 * ==> 64 sets * 64 byte = 4096 wrap value
 */
#define CACHE_SIZE (128*1024)*1000
#define CACHE_LINE_SIZE 64
#define CACHE_LINE_NR (CACHE_SIZE/CACHE_LINE_SIZE)

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
	rx_buf = mmap(0x7f0000030000,CACHE_SIZE,PROT_READ|PROT_WRITE,MAP_SHARED,fd,0);
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
 * This example illustrace the effect of the prefetcher
 * that at some points kicks in
 * _304__248__244__248__332__248__248__244__248__244__248__244__80__80__80__80__80__80__80__80__80__80
 */

#define CACHE_LINE_PER_PAGE 64*8
#define PAGE_NR 64

static void encode_cache_lines(int linenr, uint64_t value)
{
	int x;
	uint64_t tmp;

	tmp = value;
	for(x=0;x<PAGE_NR;x++){
		if(!(tmp & 0x1))
			load_cache_line((x*CACHE_LINE_PER_PAGE)+linenr);
		tmp = tmp >> 1;
	}

	tmp = value;
	for(x=0;x<PAGE_NR;x++){
		if((tmp & 0x1))
			zap_cache_line((x*CACHE_LINE_PER_PAGE)+linenr);
		tmp = tmp >> 1;
	}
}

static const uint64_t no_order[] = { 46, 10, 41, 61, 11, 13, 37, 12, 48, 59, 0, 54, 30, 7, 57, 58, 17, 16, 25, 35, 62, 15, 2, 26, 21, 39, 50, 32, 23, 36, 18, 43, 47, 45, 24, 20, 27, 29, 60, 55, 28, 3, 1, 8, 22, 53, 42, 56, 33, 19, 34, 5, 49, 31, 51, 40, 6, 38, 52, 63, 4, 14, 44, 9};

static uint64_t decode_cache_line(int linenr)
{
	int x,t;
	cycles_t t1,t2;
	uint64_t data;

	data = 0;
	for(x=0;x<PAGE_NR;x++){
		t1 = get_cycles();
		load_cache_line(((no_order[x])*CACHE_LINE_PER_PAGE)+linenr);
		if(get_cycles()-t1 > 200)
			data = data | (uint64_t)1 << no_order[x];
	}
	return data;
}


void prefetch(void(*fn)(cycles_t))
{
	uint64_t data0,data1,data;
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

	//Encode
	data0 = 0x5533555555558855;
	data1 = 0xaaaaaaaaaaaaaaaa;

	//data0 = 0xffffffffffffffff;
	//data1 = 0xffffffffffffffff;

//	data0 = 0;
//	data1 = 0;

	for(y=0;y<CACHE_LINE_PER_PAGE;y++){
		if(!(y%2))
			encode_cache_lines(y, data0);
		else
			encode_cache_lines(y, data1);
	}

	//Decode
	for(y=0;y<CACHE_LINE_PER_PAGE;y++){
		t1 = get_cycles();
		data = decode_cache_line(y);
		fprintf(stderr,"%Ld: %Lx\n",get_cycles()-t1,data);
	}
	return;
}
