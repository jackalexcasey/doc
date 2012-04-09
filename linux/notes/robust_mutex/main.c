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

struct sharedmutex{
	pthread_mutex_t mutex[MAX_MUTEX_NR];
};
static inline void* mmap_hwid_file(void)
{
	int rc,fd;
	void *ptr;
	char buf[_POSIX_PATH_MAX];

	snprintf(buf,_POSIX_PATH_MAX,"./mutex_test");
	fd = open(buf, O_RDWR, 0666);
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
volatile int flag=0;
volatile unsigned long long Gx=0,dummy,second;

void * init(void*arg)
{
	int x;
	pthread_mutexattr_t attr;
	struct sharedmutex *shmmutex;
	char *ptr = NULL;

	shmmutex =  mmap_hwid_file();
	if(!shmmutex){
		perror("mmap_hwid_file");
		return;
	}
	memset(shmmutex,0,HWID_TABLE_SIZE);
	pthread_mutexattr_init (&attr);
	pthread_mutexattr_setrobust_np (&attr, PTHREAD_MUTEX_ROBUST_NP);

	/* I'm taking a ref lock on 100 mutex */
	for(x=0;x<100;x++){
		pthread_mutex_init(&shmmutex->mutex[x], &attr);
		pthread_mutex_lock(&shmmutex->mutex[x]);
	}

	fprintf(stderr,"INIT sleeping for %d sec\n",second);
	//sleep(20);

	while(!flag){
		dummy++;
	}

	for(x=0;x<5;x++){
	for(Gx=0;Gx<(500*1024*1024);Gx++){
		dummy++;
	}
	}

// This thread had all the lock;
// It release to the first thread which perform a
//	fprintf(stderr,"BOOM!!!!!!!!\n");

/* HEre we need to make sure the quantum is expired 
 * At the momemnt the wake up happen from the dying thread that thread 
 * can be preempted (if the timeslice is exhausted)
 * equivalent to timeout???
 *
 * */
	/* 
	 * I'm holding the lock and I'm about to terminate
	 * this is going to cause a EOWNERDEAD on all waiter
	 * And every body will be woken up
	 * */
	pthread_exit(NULL);


	while(1){} 
}

void * lock(void *arg)
{
	struct sharedmutex *shmmutex;
	int rc,i,x;

	fprintf(stderr,"LOCKER\n");
	shmmutex =  mmap_hwid_file();
	if(!shmmutex){
		perror("mmap_hwid_file");
		return;
	}

	flag=1;
	/* Here I wait until the lock is release or the OWNER died */
	rc = pthread_mutex_lock(&shmmutex->mutex[13]);
	if(rc == EOWNERDEAD){
		/* Ok I have the look and I woke up part of the wake from thread1 terminating
		 * If I terminate here I have the lock so the Hash bucket will be the same
		 */
		fprintf(stderr,"pthread_mutex_lock EOWNERDEAD\n");
		for(x=0;x<5;x++){
		for(Gx=0;Gx<(500*1024*1024);Gx++){
			dummy++;
		}
		}
		pthread_exit(NULL);
	}
	fprintf(stderr,"Locker DONE\n");
	


	while(1)
		sleep(1);
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
	int fd,x;

	pthread_t tid;

	if(argc !=2)
		return -1;

	second = atoi(argv[1]);

	/* This is the naming convention */
	snprintf(ptr,_POSIX_PATH_MAX,"./mutex_test");

	/* try opening with O_EXCL and if it succeeds zero the memory
	 * by truncating to 0 */
	//if ((fd = shm_open(ptr, O_CREAT|O_RDWR|O_EXCL,
	//	S_IRWXU|S_IRWXG|S_IRWXO)) > 0) {
	if ((fd = open(ptr, O_CREAT|O_RDWR|O_EXCL,
		S_IRWXU|S_IRWXG|S_IRWXO)) > 0) {
	/* truncate file to length PCI device's memory */
	if (ftruncate(fd, HWID_TABLE_SIZE) != 0)
		FATAL("could not truncate shared file\n");
	}

	pthread_create(&tid, NULL, init, NULL);
	sleep(5);
	for(x=0;x<2;x++){
		pthread_create(&tid, NULL, lock, NULL);
	}

	while(1) sleep(1);
}

