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
#define CACHE_SIZE ((128*1024)*1000)
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
	rx_buf = malloc(CACHE_SIZE);
//	rx_buf = mmap(0x7f0000030000,CACHE_SIZE,PROT_READ|PROT_WRITE,MAP_SHARED,fd,0);
//	if(rx_buf == MAP_FAILED)
//		DIE("mmap");
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

static void touch_cache_line(int linenr)
{
	//__builtin_prefetch(&rx_buf[64*linenr]);
	rx_buf[CACHE_LINE_SIZE*linenr]++;
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


void i386_cpuid_caches () {
    int i;
    for (i = 0; i < 32; i++) {

        // Variables to hold the contents of the 4 i386 legacy registers
        uint32_t eax, ebx, ecx, edx; 

        eax = 4; // get cache info
        ecx = i; // cache id

        __asm__ (
            "cpuid" // call i386 cpuid instruction
            : "+a" (eax) // contains the cpuid command code, 4 for cache query
            , "=b" (ebx)
            , "+c" (ecx) // contains the cache id
            , "=d" (edx)
        ); // generates output in 4 registers eax, ebx, ecx and edx 

        // taken from http://download.intel.com/products/processor/manual/325462.pdf Vol. 2A 3-149
        int cache_type = eax & 0x1F; 

        if (cache_type == 0) // end of valid cache identifiers
            break;

        char * cache_type_string;
        switch (cache_type) {
            case 1: cache_type_string = "Data Cache"; break;
            case 2: cache_type_string = "Instruction Cache"; break;
            case 3: cache_type_string = "Unified Cache"; break;
            default: cache_type_string = "Unknown Type Cache"; break;
        }

        int cache_level = (eax >>= 5) & 0x7;

        int cache_is_self_initializing = (eax >>= 3) & 0x1; // does not need SW initialization
        int cache_is_fully_associative = (eax >>= 1) & 0x1;


        // taken from http://download.intel.com/products/processor/manual/325462.pdf 3-166 Vol. 2A
        // ebx contains 3 integers of 10, 10 and 12 bits respectively
        unsigned int cache_sets = ecx + 1;
        unsigned int cache_coherency_line_size = (ebx & 0xFFF) + 1;
        unsigned int cache_physical_line_partitions = ((ebx >>= 12) & 0x3FF) + 1;
        unsigned int cache_ways_of_associativity = ((ebx >>= 10) & 0x3FF) + 1;

        // Total cache size is the product
        size_t cache_total_size = cache_ways_of_associativity * cache_physical_line_partitions * cache_coherency_line_size * cache_sets;

        printf(
            "Cache ID %d:\n"
            "- Level: %d\n"
            "- Type: %s\n"
            "- Sets: %d\n"
            "- System Coherency Line Size: %d bytes\n"
            "- Physical Line partitions: %d\n"
            "- Ways of associativity: %d\n"
            "- Total Size: %zu bytes (%zu kb)\n"
            "- Is fully associative: %s\n"
            "- Is Self Initializing: %s\n"
            "\n"
            , i
            , cache_level
            , cache_type_string
            , cache_sets
            , cache_coherency_line_size
            , cache_physical_line_partitions
            , cache_ways_of_associativity
            , cache_total_size, cache_total_size >> 10
            , cache_is_fully_associative ? "true" : "false"
            , cache_is_self_initializing ? "true" : "false"
        );
    }
}

#define CACHE_SIZE ((128*1024)*1000)
#define CACHE_LINE_SIZE 64

#define N_SIZE_1K 512
#define LOOP_FACTOR2 16*128*3*5
void cache_sizing(void(*fn)(cycles_t))
{
	uint64_t data0,data1,data;
	int x,y,z,g,t, m,n;

	cycles_t t1,t2;
	open_c();

	i386_cpuid_caches();
	return;
#if 0

Since every number can be reduced to a product of primes, you could take the smallest prime and multiply it by itself until you get a number that goes over 1000. 

For example: 
2x2x2x2x2x2x2x2x2 = 512 

The cache size should be adjusted to something close to the highest factor number
To mazimize the factor #
_16 1_
0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 
_8 2_
0 1 0 1 0 1 0 1 0 1 0 1 0 1 0 1 
_4 4_
0 1 2 3 0 1 2 3 0 1 2 3 0 1 2 3 
_2 8_
0 1 2 3 4 5 6 7 0 1 2 3 4 5 6 7 
_1 16_
0 1 2 3 4 5 6 7 8 9 10 11 12 13 14 15

#endif

	/* first we find all the factor */
	for(x=LOOP_FACTOR2;x>=1;x--){
		if(LOOP_FACTOR2%x == 0){
			y = LOOP_FACTOR2/x;
			/* Trigger constans deepth loop */
			t1 = get_cycles();
			if(x>y){
				for(m=0;m<x;m++){
					for(n=0;n<y;n++){
						load_cache_line(n);
	//					fprintf(stderr,"%d ",n);
					}
				}
			}
			else{
				for(m=0;m<x;m++){
					for(n=0;n<y;n++){
						load_cache_line(n);
						//fprintf(stderr,"%d ",n);
					}
				}
			}
		}
	}

	/* first we find all the factor */
	for(x=LOOP_FACTOR2;x>=1;x--){
		if(LOOP_FACTOR2%x == 0){
			y = LOOP_FACTOR2/x;
	//		fprintf(stderr,"_%d %d_\n",x,y);
			/* Trigger constans deepth loop */
			t1 = get_cycles();
			if(x>y){
				for(m=0;m<x;m++){
					for(n=0;n<y;n++){
						load_cache_line(n);
	//					fprintf(stderr,"%d ",n);
					}
				}
			}
			else{
				for(m=0;m<x;m++){
					for(n=0;n<y;n++){
						load_cache_line(n);
						//fprintf(stderr,"%d ",n);
					}
				}
			}
			fprintf(stderr,"%Ld \n",get_cycles()-t1);
		}
	}
	return ;

	//16z is 1Kb
	for(z=4096,y=1;z>=1;z=z>>1,y=y<<1){
		fprintf(stderr, "%d %d\n",z,y);
		t1 = get_cycles();
		// z time y pages
		for(x=0;x<z;x++){
			for(t=0;t<y;t++){
				load_cache_line(t);
			}
		}
//		fprintf(stderr,"%Ld \n",get_cycles()-t1);
	}
	return;


	/*
	 * first of all let's pagein all the page into memory
	 */
	for(x=0;x<CACHE_SIZE;x=x+CACHE_LINE_SIZE){
		dummy = rx_buf[x];
	}
	mb();

	/*
	 * The lets measure the access time of an array that increase in size
	 * There should be a non-linear jump when we cross cache L1 L2 L3
	 */
	
	/* First pass we fill the cache */
	for(y=0;y<256;y++){
		for(x=0;x<64;x++)
			load_cache_line(x+(y*64));
		mb();
	}

	/* Then we measure the access time */
	for(y=0;y<1024;y++){
		t1 = get_cycles();
		for(x=0;x<y;x++)
			touch_cache_line(x);
		mb();
		fprintf(stderr,"%Ld \n",get_cycles()-t1);
	}



	return;


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

