/*
 * Copyright (C) 2010 Cisco Systems
 * Author: Etienne Martineau <etmartin@cisco.com>
 *
 * This work is licensed under the terms of the GNU GPL, version 2.
 */

#include "config.h"

#define mb() asm volatile("mfence":::"memory")

# define __force __attribute__((force))
static inline void clflush(volatile void *__p)
{
	asm volatile("clflush %0" : "+m" (*(volatile char __force *)__p));
}

unsigned char *rx_buf = NULL;
volatile unsigned char dummy;

unsigned char data[DATA_PACKET_SIZE];

#define CACHE_LINE_NR 64*16
#define CACHE_LINE_SIZE 64
#define CACHE_SIZE CACHE_LINE_SIZE*CACHE_LINE_NR*64

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
static void encode_cache_lines(int linenr, uint64_t value)
{
	int x;
	uint64_t tmp;

	tmp = value;
	for(x=0;x<64;x++){
		if(!(tmp & 0x1))
			load_cache_line((x*CACHE_LINE_NR)+linenr);
		tmp = tmp >> 1;
	}

	tmp = value;
	for(x=0;x<64;x++){
		if((tmp & 0x1))
			zap_cache_line((x*CACHE_LINE_NR)+linenr);
		tmp = tmp >> 1;
	}
}

const uint64_t no_order[] = { 46, 10, 41, 61, 11, 13, 37, 12, 48, 59, 0, 54, 30, 7, 57, 58, 17, 16, 25, 35, 62, 15, 2, 26, 21, 39, 50, 32, 23, 36, 18, 43, 47, 45, 24, 20, 27, 29, 60, 55, 28, 3, 1, 8, 22, 53, 42, 56, 33, 19, 34, 5, 49, 31, 51, 40, 6, 38, 52, 63, 4, 14, 44, 9};

static uint64_t decode_cache_line(int linenr)
{
	int x,t;
	cycles_t t1,t2;
	uint64_t data;

	data = 0;
	for(x=0;x<64;x++){
		t1 = get_cycles();
		load_cache_line(((no_order[x])*CACHE_LINE_NR)+linenr);
		if(get_cycles()-t1 > 200)
			data = data | (uint64_t)1 << no_order[x];
	}
	return data;
}

/*
 * The time spent in the RX vs TX vary function of the data that we
 * modulate.
 * Encoding all 0xffs involve zapping cache line only so this is fast
 * This is also function of going from state A to state B and this
 * is affecting the load_cache time
 */
void modulate_cache(cycles_t init)
{
	int y;
	uint64_t dat, *dat_ptr;

	dat_ptr = (uint64_t*)get_frame_ptr();
	
	//This is the encoding part
	if(transmitter){
		for(y=0;y<CACHE_LINE_NR;y++){
			encode_cache_lines(y, dat_ptr[y]);
		}

	}
	else{
 		/* Here the receiver need to run _after_ the transmitter */
		calibrated_ldelay(500000);

//		fprintf(stderr,"_%Ld_\n",get_cycles());a

		for(y=0;y<CACHE_LINE_NR;y++){
			dat = decode_cache_line(y);
			dat_ptr[y] = dat;

//			if(!(y%32))
//				fprintf(stderr,"%llx\n",data);
		}

	}

	screen_dump((unsigned char*)dat_ptr);
}

void cache_open_channel(unsigned long long pci_mem_addr)
{
	int fd;

	if(pci_mem_addr){
		/*
		 * Instead of relying on KSM and find which page are shared
		 * we could just use this SHM and mmap it on both side
		 * -device ivshmem,shm=test,size=500m
		 */
		fprintf(stderr,"Using provide address cookie 0x%llx\n",pci_mem_addr);
		fd = open ( "/dev/mem", O_RDWR);
		if(fd<0)
			DIE("cannot open /dev/mem");
		rx_buf = mmap(0x7f0000030000, CACHE_SIZE, PROT_READ|PROT_WRITE, MAP_SHARED, fd, (off_t)pci_mem_addr);
		if(rx_buf == MAP_FAILED)
			DIE("mmap");
		fprintf(stderr, "rx_buf mmap ptr %p\n",rx_buf);
	}
	else{
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

	display_init();

}
