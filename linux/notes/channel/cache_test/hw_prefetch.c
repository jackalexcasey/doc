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
#define CACHE_SIZE (128*1024)*100
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

#define CACHE_LINE_PER_PAGE 64
#define PAGE_NR 64

static void encode_cache_lines(int linenr, u64 value)
{
	int x;
	u64 tmp;

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

static u64 decode_cache_line(int linenr)
{
	int x,t;
	cycles_t t1,t2;
	u64 data;

	data = 0;
	for(x=0;x<PAGE_NR;x++){
		t1 = get_cycles();
		load_cache_line((x*CACHE_LINE_PER_PAGE)+linenr);
		mb();
		if(get_cycles()-t1 > 200)
			data = data|0x1;
		data = data << 1;
	}
	return data;
#if 0

	if(measure_cache_line_bit(linenr+300))
		tmp = tmp | 1 <<3;
	if(measure_cache_line_bit(linenr+700))
		tmp = tmp | 1 <<7;
	if(measure_cache_line_bit(linenr+200))
		tmp = tmp | 1 <<2;
	if(measure_cache_line_bit(linenr+600))
		tmp = tmp | 1 <<6;
	if(measure_cache_line_bit(linenr+400))
		tmp = tmp | 1 <<4;
	if(measure_cache_line_bit(linenr+100))
		tmp = tmp | 1 <<1;
	if(measure_cache_line_bit(linenr+500))
		tmp = tmp | 1 <<5;
	if(measure_cache_line_bit(linenr+0))
		tmp = tmp | 1 <<0;
	
	return tmp;
#endif

}


void prefetch(void(*fn)(cycles_t))
{
	u64 data0,data1,data;
	int x,y;

	open_c();

	//Encode
	data0 = 0x5555555555555555;
	data1 = 0xaaaaaaaaaaaaaaaa;

	for(y=0;y<CACHE_LINE_PER_PAGE;y++){
		if(!(y%2))
			encode_cache_lines(y, data0);
		else
			encode_cache_lines(y, data1);
	}

	//Decode
	for(y=0;y<CACHE_LINE_PER_PAGE;y++){
		data = decode_cache_line(y);
		fprintf(stderr,"%Lx\n",data);
	}
	return;
}
