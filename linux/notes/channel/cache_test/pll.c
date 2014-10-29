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

	if(measure_cache_line_access_time(linenr+300)>100)
		tmp = tmp | 1 <<3;
	if(measure_cache_line_access_time(linenr+700)>100)
		tmp = tmp | 1 <<7;
	if(measure_cache_line_access_time(linenr+200)>100)
		tmp = tmp | 1 <<2;
	if(measure_cache_line_access_time(linenr+600)>100)
		tmp = tmp | 1 <<6;
	if(measure_cache_line_access_time(linenr+400)>100)
		tmp = tmp | 1 <<4;
	if(measure_cache_line_access_time(linenr+100)>100)
		tmp = tmp | 1 <<1;
	if(measure_cache_line_access_time(linenr+500)>100)
		tmp = tmp | 1 <<5;
	if(measure_cache_line_access_time(linenr+0)>100)
		tmp = tmp | 1 <<0;
	
	return tmp;
}


void pll(void(*fn)(cycles_t))
{
	int x,y;
	int array[CACHE_LINE_NR];
	unsigned char value;
	
	open_c();

#if 1
	for(x=0;x<0xff;x++){
		encode_cache_lines(0,x);
		value = decode_cache_line(0);
		fprintf(stderr,"_%x:%x_\n",x,value);
	}

	return;
#endif

#if 1
//	for(x=0;x<8;x++){
//		load_cache_line(x);
//		zap_cache_line(x);
//	}
	
#if 0
	encode_cache_lines(0,0x1f);
	value = decode_cache_line(0);
	fprintf(stderr,"%x\n",value);


	return;

#else
/*
	load_cache_line(0);
	load_cache_line(1);
	load_cache_line(2);
	load_cache_line(3);
	load_cache_line(4);
	load_cache_line(5);
	load_cache_line(6);
	load_cache_line(7);

	mb();
	zap_cache_line(0);
	zap_cache_line(1);
	zap_cache_line(2);
	zap_cache_line(3);
	zap_cache_line(4);
	zap_cache_line(5);
	zap_cache_line(6);
	zap_cache_line(7);
*/
#endif
	encode_cache_lines(0,0xc0);

#if 0
//	load_cache_line(0);
//	load_cache_line(1);
//	load_cache_line(2);
	load_cache_line(3);
	load_cache_line(4);
	load_cache_line(5);
	load_cache_line(6);
	load_cache_line(7);

	zap_cache_line(0);
	zap_cache_line(1);
	zap_cache_line(2);
	//zap_cache_line(3);
	//zap_cache_line(4);
	//zap_cache_line(5);
	//zap_cache_line(6);
	//zap_cache_line(7);
#endif

	array[3] = measure_cache_line_access_time(3);
	array[7] = measure_cache_line_access_time(7);
	array[2] = measure_cache_line_access_time(2);
	array[6] = measure_cache_line_access_time(6);
	array[4] = measure_cache_line_access_time(4);
	array[1] = measure_cache_line_access_time(1);
	array[5] = measure_cache_line_access_time(5);
	array[0] = measure_cache_line_access_time(0);

	for(x=0;x<8;x++){
		fprintf(stderr,"_%d:%d_",x,array[x]);
	}
	return;
#endif

#if 1
	for(x=0;x<CACHE_LINE_NR;x=x+4){
		for(y=0;y<4;y++){
	//		if(!((x+y)%2))
				zap_cache_line(x+y);
	//		else
	//			load_cache_line(x+y);
		}
#if 0
		for(y=0;y<8;y++){
			/*The linear progression mixup the date because of auto prefetch */
			array[x+y] = measure_cache_line_access_time(x+y);
		}
#else
		array[x+3] = measure_cache_line_access_time(x+3);
//		array[x+7] = measure_cache_line_access_time(x+7);
		array[x+2] = measure_cache_line_access_time(x+2);
//		array[x+6] = measure_cache_line_access_time(x+6);
//		array[x+4] = measure_cache_line_access_time(x+4);
		array[x+1] = measure_cache_line_access_time(x+1);
//		array[x+5] = measure_cache_line_access_time(x+5);
		array[x+0] = measure_cache_line_access_time(x+0);
#endif

	}

	for(x=0;x<CACHE_LINE_NR;x++){
		fprintf(stderr,"\t _%Ld_",array[x]);
	}

#else
	for(x=0;x<CACHE_LINE_NR;x=x+4){
		//This is the encoding part
		encode_cache_lines(x, x);
		//This is the decoding part
		array[x] = decode_cache_line(x);
	}

	for(x=0;x<CACHE_LINE_NR;x=x+8){
		fprintf(stderr,"\t _%Ld_",array[x]);
	}
#endif

}
