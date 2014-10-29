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

#define mb() asm volatile("mfence":::"memory")

# define __force __attribute__((force))
static inline void clflush(volatile void *__p)
{
	asm volatile("clflush %0" : "+m" (*(volatile char __force *)__p));
}

unsigned char *rx_buf = NULL;
volatile unsigned char dummy;

/*
 * Suppose 32Kb L1 cache ,8way ,64byte per line
 * ==> 32kb / 64byte == 512 lines
 * ==> 512 lines / 8way == 64 sets
 * ==> 64 sets * 64 byte = 4096 wrap value
 */
#define CACHE_SIZE (128*1024)
#define CACHE_LINE_SIZE 64
#define CACHE_LINE_NR (CACHE_SIZE/CACHE_LINE_SIZE)

void open_c(void)
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

void zap_cache_line(int linenr)
{
	clflush(&rx_buf[CACHE_LINE_SIZE*linenr]);
//	rx_buf[set] = 0xff;
	mb();
}

void load_cache_line(int linenr)
{
	//__builtin_prefetch(&rx_buf[64*linenr]);
	dummy = rx_buf[CACHE_LINE_SIZE*linenr];
	mb();
}

cycles_t measure_cache_line_access_time(int linenr)
{
	cycles_t t1;

	t1 = get_cycles();
	dummy = rx_buf[CACHE_LINE_SIZE*linenr];
	mb();
	return get_cycles() - t1;
}

int measure_cache_line_bit(int linenr)
{
	cycles_t t1;

	t1 = get_cycles();
	dummy = rx_buf[CACHE_LINE_SIZE*linenr];
	mb();
	if((get_cycles() - t1)>100)
		return 1;
	return 0;
}

/*
 * Logical 1 is a slow line i.e. zap_cache_line
 * Logical 0 is a fast line i.e. load_cache_line
 *
 * Here the load cache can bring other stuff hence we
 * zap all the other line at then end only
 */
void encode_cache_lines(int linenr, unsigned char value)
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
 * Here we measure out of order
 */
unsigned char decode_cache_line(int linenr)
{
	int x,t;
	unsigned char tmp=0;

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
}

void modulate_cache(cycles_t init)
{
	int x;
	unsigned char value[16];
	
	//This is the encoding part
	if(transmitter){
		x = 0x55;
		encode_cache_lines(0,x);
		encode_cache_lines(10,x);
		encode_cache_lines(20,x);
		encode_cache_lines(30,x);
		encode_cache_lines(40,x);
		encode_cache_lines(50,x);
		encode_cache_lines(60,x);
		encode_cache_lines(70,x);
	}
	else{
		value[0] = decode_cache_line(0);
		value[1] = decode_cache_line(10);
		value[2] = decode_cache_line(20);
		value[3] = decode_cache_line(30);
		value[4] = decode_cache_line(40);
		value[5] = decode_cache_line(50);
		value[6] = decode_cache_line(60);
		value[7] = decode_cache_line(70);
	}

//	if(!ascii)
//		screen_dump(data);

	if(!transmitter){
		fprintf(stderr,"\r_%x:%x:%x:%x:%x:%x:%x:%x_",value[0],value[1],value[2],value[3],
			value[4],value[5],value[6],value[7]);
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

	if(transmitter)
		tx_init();
	else
		rx_init();

}
