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

int type=0;
static int mutexnr=-1;
struct sharedmutex{
	pthread_mutex_t mutex[MAX_MUTEX_NR];
};
char filename[256];

static inline void* mmap_hwid_file(void)
{
	int rc,fd;
	void *ptr;

	if(type==0)
		fd = open(filename, O_RDWR, S_IRWXU|S_IRWXG|S_IRWXO);
	else
		fd = shm_open(filename, O_RDWR, S_IRWXU|S_IRWXG|S_IRWXO);

	if(fd<0){
		rc = errno;
		goto err;
	}

	ptr = mmap(NULL,HWID_TABLE_SIZE,PROT_READ|PROT_WRITE,MAP_SHARED,fd,0);
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

void init(void)
{
	int x;
	pthread_mutexattr_t attr;
	struct sharedmutex *shmmutex;

	shmmutex =  mmap_hwid_file();
	if(!shmmutex){
		perror("mmap_hwid_file");
		return;
	}
	memset(shmmutex,0,HWID_TABLE_SIZE);

	pthread_mutexattr_init (&attr);
	pthread_mutexattr_setrobust_np (&attr, PTHREAD_MUTEX_ROBUST_NP);

	for(x=0;x<mutexnr;x++){
		pthread_mutex_init(&shmmutex->mutex[x], &attr);
		pthread_mutex_lock(&shmmutex->mutex[x]);
	}

	munmap(shmmutex,HWID_TABLE_SIZE);
	fprintf(stderr,"Holding %d mutex under %s%s BUT dropped the mmap\n",mutexnr,type?"/dev/shm/":"",filename);
	while(1) sleep(1);
}

void usage()
{
	fprintf(stderr,"./robust_mutex mutexnr(max %d) type [0 std, 1 shm] filename\n",MAX_MUTEX_NR);
	exit(-1);
}

/* 
 * We are trying to get the dying process to sleep in get_user...
 * SO we unmap the mutex area then the list parsing in the kernel will fault
 */
int main (int argc, char*argv[])
{
	int fd;
	char buf[256];

	if(argc <4)
		usage();

	mutexnr = atoi(argv[1]);
	if(mutexnr >= MAX_MUTEX_NR || mutexnr<0)
		usage();

	type = atoi(argv[2]);

	snprintf(filename,_POSIX_PATH_MAX,argv[3]);

	if(type){
		snprintf(buf,_POSIX_PATH_MAX,"/dev/shm/%s",filename);
		unlink(buf);
	}
	else
		unlink(filename);

	if(type==0)
		fd = open(filename, O_CREAT|O_RDWR|O_EXCL, S_IRWXU|S_IRWXG|S_IRWXO);
	else
		fd = shm_open(filename, O_CREAT|O_RDWR|O_EXCL, S_IRWXU|S_IRWXG|S_IRWXO);

	if (ftruncate(fd, HWID_TABLE_SIZE) != 0)
		FATAL("could not truncate shared file\n");

	init();

	return -1;
}

