/*
 * Copyright (C) 2010 Cisco Systems
 * Author: Etienne Martineau <etmartin@cisco.com>
 *
 * This work is licensed under the terms of the GNU GPL, version 2.
 */

#include "config.h"

#define U64_BITS_NR 64 /* number of bits in a u64 */
#define CACHE_LINE_SIZE 64 /* One cache line is 64 bytes */
#define CACHE_LINE_PER_PAGE (4096/CACHE_LINE_SIZE) /* There is 64 cache line per page */

#define CACHE_SIZE (1024*1024*12) /* 12 Mb L3 cache */
#define CACHE_BITS_NR (CACHE_SIZE / CACHE_LINE_SIZE) /* amount of bits available in the cache 1line per bits */
#define CACHE_U64_NR (CACHE_BITS_NR/64)
#define CACHE_NR_OF_64_U64 (CACHE_U64_NR/64)

#define DIV_ROUND_UP(n,d) (((n) + (d) - 1) / (d))

#define BITS_PER_BYTE       8
#define BITS_PER_LONG 64
#define BITS_TO_LONGS(nr)   DIV_ROUND_UP(nr, BITS_PER_BYTE * sizeof(long))
#define small_const_nbits(nbits) \
	(__builtin_constant_p(nbits) && (nbits) <= BITS_PER_LONG)

#define DECLARE_BITMAP(name,bits) \
	unsigned long name[BITS_TO_LONGS(bits)]

typedef unsigned int u32;

#define BITOP_ADDR(x) "+m" (*(volatile long *) (x))
#define ADDR                BITOP_ADDR(addr)
static inline void __set_bit(int nr, volatile unsigned long *addr)
{
	asm volatile("bts %1,%0" : ADDR : "Ir" (nr) : "memory");
}

static inline int constant_test_bit(unsigned int nr, const volatile unsigned long *addr)
{
    return ((1UL << (nr % BITS_PER_LONG)) &
        (addr[nr / BITS_PER_LONG])) != 0;
}

static inline int variable_test_bit(int nr, volatile const unsigned long *addr)
{
    int oldbit;

    asm volatile("bt %2,%1\n\t"
             "sbb %0,%0"
             : "=r" (oldbit)
             : "m" (*(unsigned long *)addr), "Ir" (nr));

    return oldbit;
}

/**
 * test_bit - Determine whether a bit is set
 * @nr: bit number to test
 * @addr: Address to start counting from
 */
static int test_bit(int nr, const volatile unsigned long *addr);
#define test_bit(nr, addr)          \
    (__builtin_constant_p((nr))     \
     ? constant_test_bit((nr), (addr))  \
     : variable_test_bit((nr), (addr)))

#define mb() asm volatile("mfence":::"memory")

# define __force __attribute__((force))
static inline void clflush(volatile void *__p)
{
	asm volatile("clflush %0" : "+m" (*(volatile char __force *)__p));
}

unsigned char *rx_buf = NULL;
volatile unsigned char dummy;
const uint64_t no_order[] = { 46, 10, 41, 61, 11, 13, 37, 12, 48, 59, 0, 54, 30, 7, 57, 58, 17, 16, 25, 35, 62, 15, 2, 26, 21, 39, 50, 32, 23, 36, 18, 43, 47, 45, 24, 20, 27, 29, 60, 55, 28, 3, 1, 8, 22, 53, 42, 56, 33, 19, 34, 5, 49, 31, 51, 40, 6, 38, 52, 63, 4, 14, 44, 9};

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
 * DATA_PACKET_SIZE is (640X480 bits /8) ==> byte nr
 *
 * The display is divided in 4 frame because there is not enough space
 * in the cache for all of it.
 *
 * Below 1 cache line per bit
 *
 * Data size is 640X480 bits in 4 frames
 * ==> 307200 / 4 = 76800 cache lines per frame
 *
 * 76800 / 64 = 1200
 * 1200 / 64 = 18.75
 *
 * ==>For loop factor
 *    20 / 64 / 64
 *
 * // HERE in the encoding we don't use border
 * because we want the zap to take out everything else the load could have taken
 *
 *
 * Also the no_order cannot be done over Z since there is 0-64 array...
 * NOTE that with malloc memory no_order help A LOT
 */
#define TOP_FACTOR 20
#define MIDDLE_FACTOR 64
#define BOTTOM_FACTOR 64

void encode_data(uint64_t *dat_ptr, int frame_nr)
{
	int x,y,z;
	int b,b_norder;
	uint64_t tmp;

	for(z=0;z<TOP_FACTOR;z++){
	for(y=0;y<MIDDLE_FACTOR;y++){
		b=(z*MIDDLE_FACTOR)+y;
		b_norder = z*MIDDLE_FACTOR+no_order[y];

		//64 bits
		if(!frame_nr && b_norder==(TOP_FACTOR*MIDDLE_FACTOR)-1)
			tmp = 0xdeadbeefaa55aa55;
		else
			tmp = dat_ptr[b*4+frame_nr];
		for(x=0;x<BOTTOM_FACTOR;x++){
			if(!(tmp & 0x1))
				//load_cache_line(((no_order[x])*1280)+b_norder);
				load_cache_line((x*TOP_FACTOR*MIDDLE_FACTOR)+b_norder);
			tmp = tmp >> 1;
		}

		if(!frame_nr && b_norder==(TOP_FACTOR*MIDDLE_FACTOR)-1)
			tmp = 0xdeadbeefaa55aa55;
		else
			tmp = dat_ptr[b*4+frame_nr];
		for(x=0;x<BOTTOM_FACTOR;x++){
			if((tmp & 0x1))
				//zap_cache_line(((no_order[x])*1280)+b_norder);
				zap_cache_line((x*TOP_FACTOR*MIDDLE_FACTOR)+b_norder);
			tmp = tmp >> 1;
		}
	}
	}
}

int decode_data(uint64_t *dat_ptr, int frame_nr)
{
	int err=0;
	int x,y,z;
	int b,b_norder;
	uint64_t tmp;
	cycles_t t1;

	for(z=0;z<TOP_FACTOR;z++){
	for(y=0;y<MIDDLE_FACTOR;y++){
		b=(z*MIDDLE_FACTOR)+y;
		b_norder = z*MIDDLE_FACTOR+no_order[y];
		//64 bits
		tmp = 0;
		for(x=0;x<BOTTOM_FACTOR;x++){
			t1 = get_cycles();
			load_cache_line(((no_order[x])*TOP_FACTOR*MIDDLE_FACTOR)+b_norder);
			if(get_cycles()-t1 > 200)
				tmp = tmp | (uint64_t)1 << no_order[x];
		}
		if(tmp == 0xdeadbeefaa55aa55){
			if(frame_nr !=0)
				return -1;
			err++;
		}
		dat_ptr[b*4+frame_nr] = tmp;
	}
	}
	return err;
}

/*
 * The time spent in the RX vs TX vary function of the data that we
 * modulate.
 * Encoding all 0xffs involve zapping cache line only so this is fast
 * This is also function of going from state A to state B and this
 * is affecting the load_cache time
 * This example illustrace the effect of the prefetcher
 * that at some points kicks in
 * _304__248__244__248__332__248__248__244__248__244__248__244__80__80__80__80__80__80__80__80__80__80
 *
 * With cache modulation we canno pass the whole BW in one cycle
 * for that reason we interleave the frame
 *
 * BUT with interleave we LOOSE the PLL locking so there is an out of phase that is 
 * created.........
 */
void modulate_cache(cycles_t init)
{
	int err;
	static int frame_nr=0;
	static uint64_t *dat_ptr;
	static int sync=0;
	static int signal_period = 0;
	static int signal_strength=0;

	if(!frame_nr)
		dat_ptr = (uint64_t*)get_frame_ptr();

	if(playback)
		goto end;

	if(transmitter){ //This is the encoding part
//		memset(dat_ptr,0xffffffffffffffff,DATA_PACKET_SIZE);
//		memset(dat_ptr,0x5555555555555555,DATA_PACKET_SIZE);
		encode_data(dat_ptr, frame_nr);
	}
	else{ // This is the decoding part
syncup:
		if(sync)
			calibrated_ldelay(500000);
	
		err = decode_data(dat_ptr, frame_nr);
		if(err < 0){
			fprintf(stderr,"Out of sync\n");
			frame_nr = 0;
			sync = 0;
			goto syncup;
		}
		else{ // This is the right frame
			signal_strength = err;
			if((!sync) && (signal_strength>0)){
				fprintf(stderr,"Frame synchronized!\n");
				sync = 1;
			}
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

//	fprintf(stderr,"%d\n",sizeof(u32));
//	return;
	if(pci_mem_addr){
		/*
		 * Instead of relying on KSM and find which page are shared
		 * we could just use this SHM and mmap it on both side
		 * -device ivshmem,shm=test,size=500m
		 */
		if(pci_mem_addr == 0xdead){
			fprintf(stderr,"ANON\n");
			rx_buf = mmap(0x7f0000030000, CACHE_SIZE, PROT_READ|PROT_WRITE,  MAP_PRIVATE|MAP_ANONYMOUS, -1, 0);
#if 0
When the memory is not faulted in it point to the same PFN EVEN across process????

etmartin@etmartin-desktop:/nobackup/meta/doc/linux/notes/channel/pagemap$ ./pagemap 7341 0x7f0000030000
Big endian? 0
Vaddr: 0x7f0000030000, Page_size: 4096, Entry_size: 8
Reading /proc/7341/pagemap at 0x3f80000180
[0]0x1e [1]0x19 [2]0x0 [3]0x0 [4]0x0 [5]0x0 [6]0x0 [7]0x86 
Result: 0x860000000000191e
PFN: 0x191e
etmartin@etmartin-desktop:/nobackup/meta/doc/linux/notes/channel/pagemap$ ./pagemap 7341 0x7f0000040000
Big endian? 0
Vaddr: 0x7f0000040000, Page_size: 4096, Entry_size: 8
Reading /proc/7341/pagemap at 0x3f80000200
[0]0x1e [1]0x19 [2]0x0 [3]0x0 [4]0x0 [5]0x0 [6]0x0 [7]0x86 
Result: 0x860000000000191e
PFN: 0x191e
#endif


			//For some reason when we memset ( pagefault the behavior is totally different????? )
			//Without memset we see some noise consistent with the stream??????
			//
			//Maybe we are ending up modulating just over one page and bcos
			//the TX and RX are running one after the other we endup picking some signal?
			//SO we need a modulate_page to try this out.
			//
			memset(rx_buf,0,CACHE_SIZE);
		}
		else{
//			fprintf(stderr,"Using provide address cookie 0x%llx\n",pci_mem_addr);
			fd = open ( "/dev/mem", O_RDWR);
			if(fd<0)
				DIE("cannot open /dev/mem");
			rx_buf = mmap(0x7f0000030000, CACHE_SIZE, PROT_READ|PROT_WRITE, MAP_SHARED, fd, (off_t)pci_mem_addr);
		}
		if(rx_buf == MAP_FAILED)
			DIE("mmap");
//		fprintf(stderr, "rx_buf mmap ptr %p\n",rx_buf);
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
