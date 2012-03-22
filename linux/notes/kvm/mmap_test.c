#include <err.h>
#include <stdio.h>
#include <inttypes.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#define DPRINTF(args...)  fprintf(stderr,args);

#if 0
void main(int argc, char*argv[])
{
	int time =1;
	long long unsigned int map;
	void *ptr;
	char cmd[1024];

	if(argc < 2 )
		errx(1,"Invalid Arg");

	sscanf(argv[1],"%Lx",&map);
	if(argc ==3)
		time = atoi(argv[2]);
	
	DPRINTF("Trying to Map %Lx ",map);
	sleep(time);
	/*
	 * This should cause a TLB fill because the based is zeroed out by the kernel i.e. Accessed 
	 * On KVM this will cause an EPT violation 'kvm_page_fault'
	 */
	ptr = mmap((void*)map, getpagesize(),PROT_WRITE|PROT_READ, MAP_ANONYMOUS|MAP_PRIVATE, -1,0);
	if(ptr == MAP_FAILED)
		errx(1,"Cannot mmap %p \n",(void*)map);

	if(ptr != (void*)map)
		errx(1,"Hint didn't work %p -> %p\n",ptr, (void*)map);
	DPRINTF("DONE \n");


	DPRINTF("Accessing memory ");
	sleep(time);
	memset(ptr, 0x55, getpagesize());
	DPRINTF("DONE \n");

	DPRINTF("Display mapping");
	sleep(time);
	sprintf(cmd,"cat /proc/%d/maps \n",getpid());
	system(cmd);
	DPRINTF("DONE \n");

	/*
	 * This should cause a TLB flush
	 */
	DPRINTF("Undo mapping");
	sleep(time);
	munmap(ptr,getpagesize());
	DPRINTF("DONE \n");

	DPRINTF("Accessing memory ");
	sleep(time);
	memset(ptr, 0x55, getpagesize());
	DPRINTF("DONE \n");

while(1)
	sleep(1);

}
#endif

void main(int argc, char*argv[])
{
	int fd;
	int time =1;
	long long unsigned int map;
	void *ptr;
	char cmd[1024];
	char buffer[4096];

	if(argc < 2 )
		errx(1,"Invalid Arg");

	sscanf(argv[1],"%Lx",&map);
	if(argc ==3)
		time = atoi(argv[2]);
	
	fd = open("/dev/mem",O_RDWR);
	if(fd < 0)
		err(1,"Cannot open \n");
	
	DPRINTF("Trying to Map %Lx ",map);
	sleep(time);
	/*
	 * This should cause a TLB fill because the based is zeroed out by the kernel i.e. Accessed 
	 * On KVM this will cause an EPT violation 'kvm_page_fault'
	 */
	ptr = mmap(NULL, getpagesize(),PROT_WRITE|PROT_READ, MAP_SHARED, fd,map);
	if(ptr == MAP_FAILED)
		errx(1,"Cannot mmap %p \n",(void*)map);

	DPRINTF("Reading memory ");
	sleep(time);
	memcpy(buffer, ptr, getpagesize());
	DPRINTF("DONE \n");

	DPRINTF("Display mapping");
	sleep(time);
	sprintf(cmd,"cat /proc/%d/maps \n",getpid());
	system(cmd);
	DPRINTF("DONE \n");

	/*
	 * This should cause a TLB flush
	 */
	DPRINTF("Undo mapping");
	sleep(time);
	munmap(ptr,getpagesize());
	DPRINTF("DONE \n");

while(1)
	sleep(1);

}

