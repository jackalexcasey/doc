#include <unistd.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <stdarg.h>
#include <stdio.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <sys/stat.h>        /* For mode constants */
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#define __USE_GNU /* Needed for PTHREAD_MUTEX_ROBUST_NP */
#include <pthread.h>

#ifndef _POSIX_PATH_MAX
#define _POSIX_PATH_MAX 256
#endif


/* need that for the macro... */
#define PAGE_SIZE 4096

#define __ALIGN_MASK(x,mask)    (((x)+(mask))&~(mask))
#define ALIGN(x,a)      __ALIGN_MASK(x,(typeof(x))(a)-1)
#define PAGE_ALIGN(addr) ALIGN(addr, PAGE_SIZE)

#define HWID_TABLE_SIZE (1<<20) /* 1Mb */
#define MAX_MUTEX_NR (int)(HWID_TABLE_SIZE/sizeof(pthread_mutex_t))

# define FATAL(format, ...) do {\
     fprintf(stderr, "Fatal %s %d " format,__FILE__, __LINE__, ## __VA_ARGS__);\
     exit(1);\
 }while(0)

static char filename[256];
int type=0;
int size=0;

static inline void* mmap_hwid_file(void)
{
	int rc,fd;
	void *ptr;
	struct stat buf;

	if(stat(filename, &buf))
		return NULL;

	size = buf.st_size;
	fprintf(stderr, "mmap_hwid_file %s %d\n",filename,size);

	if(type==0)
		fd = open(filename, O_RDWR, S_IRWXU|S_IRWXG|S_IRWXO);
	else
		fd = shm_open(filename, O_RDWR, S_IRWXU|S_IRWXG|S_IRWXO);

	if(fd<0){
		rc = errno;
		goto err;
	}

	ptr = mmap(NULL,size,PROT_READ|PROT_WRITE,MAP_SHARED,fd,0);
	if(ptr == MAP_FAILED){
		rc = errno;
		goto err1;
	}

	close(fd);
	return ptr;

err1:
	close(fd);
err:
	errno = rc;
	return NULL;
}

void usage()
{
	fprintf(stderr,"./robust_mutex mutex filename [extra arg mean fault in]\n");
	exit(-1);
}

/*
 * The core dump takes the RSS (from statm)
 * The mmap only grow VMsize 
 * VMRSS is the resident size
 * The kernel will walk the PTE for the process and for every page RSS pipe it to user space
 *
 * Basic lock / died sequence
 *
 */
//unsigned char data;
int main (int argc, char*argv[])
{
	int x;
	unsigned char *ptr;
	char cmd[256];

	if(argc <2)
		usage();

	snprintf(filename,_POSIX_PATH_MAX,argv[1]);
	ptr = mmap_hwid_file();
	if(!ptr)
		return -1;
	sprintf(cmd,"echo 0x3f > /proc/%d/coredump_filter\n",getpid());
	system(cmd);
	if(argc ==3 ){
		for(x=0;x<size;x++){
			ptr[x] = x;
		}
	}
	while(1) sleep(1);

}
#if 0
echo 0x3f >/proc/598/coredump_filter

/proc/PID/coredump_filter
       bit 0  Dump anonymous private mappings.
       bit 1  Dump anonymous shared mappings.
       bit 2  Dump file-backed private mappings.
       bit 3  Dump file-backed shared mappings.
       bit 4 (since Linux 2.6.24)
              Dump ELF headers.
       bit 5 (since Linux 2.6.28)
              Dump private huge pages.
       bit 6 (since Linux 2.6.28)
              Dump shared huge pages.

By default, the following bits are set: 0, 1, 4 (if the CONFIG_CORE_DUMP_DEFAULT_ELF_HEADERS kernel configuration option is enabled), and 5. The value of this file is displayed in hexadecimal. (The default value is thus displayed as 33.) Memory-mapped I/O pages such as frame buffer are never dumped, and virtual DSO pages are always dumped, regardless of the coredump_filter value.
#endif

