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

static cycles_t measure_cache_line_access_time(int linenr)
{
	cycles_t t1;

	t1 = get_cycles();
	dummy = rx_buf[CACHE_LINE_SIZE*linenr];
	mb();
	return get_cycles() - t1;
}


/*
 * Logical 1 is a slow line i.e. zap_cache_line
 * Logical 0 is a fast line i.e. load_cache_line
 *
 * Here the load cache can bring other stuff hence we
 * zap all the other line at then end only
 */
static void encode_cache_lines(int linenr, unsigned char value)
{
	int x;
	unsigned char tmp;


	tmp = value;
	for(x=0;x<8;x++){
		if(!(tmp & 0x1))
			load_cache_line(linenr+x*100);
		tmp = tmp >>1;
	}

	tmp = value;
	for(x=0;x<8;x++){
		if(tmp & 0x1)
			zap_cache_line(linenr+x*100);
		tmp = tmp >>1;
	}
}

/*
 * This example illustrace the effect of the prefetcher
 * that at some points kicks in
 * _304__248__244__248__332__248__248__244__248__244__248__244__80__80__80__80__80__80__80__80__80__80
 */

#define CACHE_LINE_PER_PAGE 64
#define PAGE_NR 64
void prefetch(void(*fn)(cycles_t))
{
	int x,y,z;
	cycles_t t1,t2;

	open_c();

	//Encode
	for(y=0;y<CACHE_LINE_PER_PAGE;y++){
		for(x=0;x<PAGE_NR;x++){
			if(!(x%2))
				zap_cache_line((x*CACHE_LINE_PER_PAGE)+y);
			else
				load_cache_line((x*CACHE_LINE_PER_PAGE)+y);
		}
	}

	//Decode
	for(y=0;y<CACHE_LINE_PER_PAGE;y++){
		for(x=0;x<PAGE_NR;x++){
			t1 = get_cycles();
			load_cache_line((x*CACHE_LINE_PER_PAGE)+y);
			t2 = get_cycles();
			fprintf(stderr,"\t_%Ld_",t2-t1);
			if(!(x%16))
				fprintf(stderr,"\n");
		}
	}
	return;
}
