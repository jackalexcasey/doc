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

#define HWID_TABLE_SIZE (1<<20) /* 1Mb */
#define MAX_MUTEX_NR (int)(HWID_TABLE_SIZE/sizeof(pthread_mutex_t))

# define FATAL(format, ...) do {\
     fprintf(stderr, "Fatal %s %d " format,__FILE__, __LINE__, ## __VA_ARGS__);\
     exit(1);\
 }while(0)

struct sharedmutex{
	pthread_mutex_t mutex[MAX_MUTEX_NR];
};
static int mutexnr=1;

static inline void* mmap_hwid_file(void)
{
	int rc,fd;
	void *ptr;
	char buf[_POSIX_PATH_MAX];

	snprintf(buf,_POSIX_PATH_MAX,"/mutex_test");
	fd = shm_open(buf, O_RDWR, 0666);
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

	fprintf(stderr,"INIT %d\n",mutexnr);
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
	while(1) sleep(1);
}

void lock(int nr)
{
	int rc;
	struct sharedmutex *shmmutex;

	fprintf(stderr,"LOCK %d\n",nr);
	shmmutex =  mmap_hwid_file();
	if(!shmmutex){
		perror("mmap_hwid_file");
		return;
	}
	rc = pthread_mutex_lock(&shmmutex->mutex[nr]);
	if(rc == EOWNERDEAD)
		fprintf(stderr,"pthread_mutex_lock EOWNERDEAD\n");
}

void usage()
{
	fprintf(stderr,"init: ./robust_mutex mutexnr(max %d)\n",MAX_MUTEX_NR);
	fprintf(stderr,"lock: ./robust_mutex 0 nr\n");
	exit(-1);
}

int main (int argc, char*argv[])
{
	char ptr[256];
	int fd;

	if(argc <2)
		usage();

	mutexnr = atoi(argv[1]);
	if(mutexnr >= MAX_MUTEX_NR || mutexnr<0)
		usage();
		
	/* This is the naming convention */
	snprintf(ptr,_POSIX_PATH_MAX,"/mutex_test");

	/* try opening with O_EXCL and if it succeeds zero the memory
	 * by truncating to 0 */
	if ((fd = shm_open(ptr, O_CREAT|O_RDWR|O_EXCL,
		S_IRWXU|S_IRWXG|S_IRWXO)) > 0) {
	/* truncate file to length PCI device's memory */
	if (ftruncate(fd, HWID_TABLE_SIZE) != 0)
		FATAL("could not truncate shared file\n");
	}

	if(argc==2)
		init();	
	else
		lock(atoi(argv[2]));	
}
