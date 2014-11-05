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

#define CACHE_LINE_NR (64*16+256)
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
	//	load_cache_line((x*CACHE_LINE_NR)+linenr);
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
 *
 * With cache modulation we canno pass the whole BW in one cycle
 * for that reason we interleave the frame
 *
 * BUT with interleave we LOOSE the PLL locking so there is an out of phase that is 
 * created.........
 */
void modulate_cache(cycles_t init)
{
	int y;
	uint64_t dat;
	static int frame_nr=0;
	static uint64_t *dat_ptr;
	static int sync=0;
	static int signal_period = 0;
	static int signal_strength=0;

	if(!frame_nr)
		dat_ptr = (uint64_t*)get_frame_ptr();

	if(playback)
		goto end;

	//This is the encoding part
	if(transmitter){
		for(y=0;y<CACHE_LINE_NR;y++){
			if(pattern){
//				encode_cache_lines(y, 0xffffffffffffffff);
				encode_cache_lines(y, 0xff00ff00ff00ff00);
			}
			else
				encode_cache_lines(y, dat_ptr[y*4+frame_nr]);
		}
		if(!frame_nr) // At the end of frame 0 we issue the magic marker
			encode_cache_lines(CACHE_LINE_NR-1,0xdeadbeefaa55aa55);
	}
	else{
 		/* Here the receiver need to run _after_ the transmitter */
syncup:
		if(sync)
			calibrated_ldelay(500000);

		for(y=0;y<CACHE_LINE_NR;y++){
			dat = decode_cache_line(y);
			if(dat == 0xdeadbeefaa55aa55){
				if(frame_nr !=0){ // Wrong frame
					fprintf(stderr,"Out of sync\n");
					frame_nr = 0;
					sync = 0;
					goto syncup;
				}
				else{ // This is the right frame
					signal_strength++;
					if(!sync){
						fprintf(stderr,"Frame synchronized!\n");
						sync = 1;
					}
				}
			}
			dat_ptr[y*4+frame_nr] = dat;
		}
		if(!frame_nr){
			signal_period++;
			if(signal_period == 30){
				if(!signal_strength){
					sync = 0;
					fprintf(stderr,"NO carrier\n");
				}
				signal_period = 0;
				signal_strength = 0;

			}
		}
	}
//	fprintf(stderr,"Frame #%d, %Ld\n",frame_nr, get_cycles()-init);

end:
	frame_nr++;
	if(frame_nr==4){
		frame_nr = 0;
		dump_frame((unsigned char*)dat_ptr);
	}
}

void cache_open_channel(unsigned long long pci_mem_addr)
{
	int fd;
	char name[64];

	if(pci_mem_addr){
		/*
		 * Instead of relying on KSM and find which page are shared
		 * we could just use this SHM and mmap it on both side
		 * -device ivshmem,shm=test,size=500m
		 */
		if(pci_mem_addr == 0xdead){
			fprintf(stderr,"ANON\n");
			rx_buf = mmap(0x7f0000030000, CACHE_SIZE, PROT_READ|PROT_WRITE,  MAP_PRIVATE|MAP_ANONYMOUS, -1, 0);
		}
		else{
			fprintf(stderr,"Using provide address cookie 0x%llx\n",pci_mem_addr);
			fd = open ( "/dev/mem", O_RDWR);
			if(fd<0)
				DIE("cannot open /dev/mem");
			rx_buf = mmap(0x7f0000030000, CACHE_SIZE, PROT_READ|PROT_WRITE, MAP_SHARED, fd, (off_t)pci_mem_addr);
		}
		if(rx_buf == MAP_FAILED)
			DIE("mmap");
		fprintf(stderr, "rx_buf mmap ptr %p\n",rx_buf);
	}
	else{
		//When the name are different i.e. mmap to different physical Address
		//the effect is very similar to the malloc case
		if(transmitter)
			sprintf(name,"channelrx");
		else
			sprintf(name,"channelrx");
		if ((fd = shm_open(name, O_CREAT|O_RDWR,
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
		 * L3 is not tagged by Virtual addr since when we set the hint to NULL it
		 * work still
		 */
		rx_buf = mmap(0x7f0000030000,CACHE_SIZE,PROT_READ|PROT_WRITE,MAP_SHARED,fd,0);
		if(rx_buf == MAP_FAILED)
			DIE("mmap");
		fprintf(stderr, "rx_buf mmap ptr %p\n",rx_buf);
	}

	display_init();

}
