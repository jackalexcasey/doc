/*
 * Copyright (C) 2010 Cisco Systems
 * Author: Etienne Martineau <etmartin@cisco.com>
 *
 * This work is licensed under the terms of the GNU GPL, version 2.
 */

#include "config.h"

extern int transmitter;
extern int ascii;
extern unsigned char Untitled_bits[];
extern int screen_dump(unsigned char *data);
extern int screen_init(int w, int h);

#define PIXEL_WIDTH				640
#define PIXEL_HEIGHT			480
#define DATA_PACKET_SIZE 		(PIXEL_WIDTH*PIXEL_HEIGHT)/8
unsigned char data[DATA_PACKET_SIZE];

/*
 * Suppose 32Kb L1 cache ,8way ,64byte per line
 * ==> 32kb / 64byte == 512 lines
 * ==> 512 lines / 8way == 64 sets
 * ==> 64 sets * 64 byte = 4096 wrap value
 */

#define mb()    asm volatile("mfence":::"memory")
volatile unsigned char dummy;
unsigned char *rx_buf = NULL;

/* 
 * this is flushing one cache line pointed by this addr
 */
# define __force    __attribute__((force))
static inline void clflush(volatile void *__p)
{
	asm volatile("clflush %0" : "+m" (*(volatile char __force *)__p));
}

static void zap_cache_line(int linenr)
{
	clflush(&rx_buf[64*linenr]);
//	rx_buf[set] = 0xff;
	mb();
}

static void load_cache_line(int linenr)
{
	__builtin_prefetch(&rx_buf[64*linenr]);
//	dummy = rx_buf[64*linenr];
	mb();
}

static cycles_t measure_cache_line_access_time(int linenr)
{
	cycles_t t1;

	t1 = get_cycles();
	dummy = rx_buf[64*linenr];
	mb();
	return get_cycles() - t1;
}

void modulate_cache(cycles_t init)
{
	int x;
	cycles_t array[512];
	
	for(x=0;x<512;x++){
		//This is the encoding part
		if(transmitter){
			if(!(x%2))
				zap_cache_line(x);
			else
				load_cache_line(x);
		}
		else
			//This is the decoding part
			array[x] = measure_cache_line_access_time(x);
	}

//	if(!ascii)
//		screen_dump(data);

	if(!transmitter){
		for(x=0;x<512;x++){
			fprintf(stderr,"\t _%Ld_",array[x]);
		}
	}
}

static void rx_init(void)
{
	if(!ascii)
		screen_init(PIXEL_WIDTH, PIXEL_HEIGHT);
}

static void tx_init(void)
{	
	int c;

	if(ascii){
		for(c=0; c<DATA_PACKET_SIZE; c++){
			data[c] = c;
		}
	}
	else{
		screen_init(PIXEL_WIDTH, PIXEL_HEIGHT);
		/* Data source is bitmap */
		memcpy(data,Untitled_bits,DATA_PACKET_SIZE);
		screen_dump(Untitled_bits);
	}
}

void open_channel(unsigned long long pci_mem_addr)
{
	int fd;

	if ((fd = shm_open("channelrx", O_CREAT|O_RDWR,
					S_IRWXU|S_IRWXG|S_IRWXO)) > 0) {
		if (ftruncate(fd, 1024*32) != 0)
			DIE("could not truncate shared file\n");
	}
	else
		DIE("Open channel");
	
	rx_buf = mmap(0x7f0000030000,1024*32,PROT_READ|PROT_WRITE,MAP_SHARED,fd,0);
	if(rx_buf == MAP_FAILED)
		DIE("mmap");
	fprintf(stderr, "rx_buf mmap ptr %p\n",rx_buf);

	if(transmitter)
		tx_init();
	else
		rx_init();
}


