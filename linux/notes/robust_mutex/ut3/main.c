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

#include "spinlock.h"

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
static struct sharedmutex *shmmutex;

static char filename[256];
static spinlock_t lock;

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

void * waitlock(void *arg)
{
	int rc,x;

	shmmutex =  mmap_hwid_file();
	if(!shmmutex){
		perror("mmap_hwid_file");
		return NULL;
	}

	/* Here I wait until the lock is release or the OWNER died */
	fprintf(stderr,"Waiting on all mutex %d\n",mutexnr);
	spin_unlock(&lock);
	for(x=0;x<mutexnr;x++){
		rc = pthread_mutex_lock(&shmmutex->mutex[x]);
		if(rc == EOWNERDEAD){
			fprintf(stderr,"mutex[%d] pthread_mutex_lock EOWNERDEAD\n",x);
		}
	}
	fprintf(stderr,"GOT ALL LOCK\n");
	return NULL;
}

void usage()
{
	fprintf(stderr,"./robust_mutex mutexnr (lock all of them) type [0 std, 1 shm] filename\n");
	exit(-1);
}

/*
 * Basic lock / died sequence
 *
 */

int main (int argc, char*argv[])
{
	int x;
	pthread_t tid[100];

	if(argc <4)
		usage();

	mutexnr = atoi(argv[1]);
	if(mutexnr >= MAX_MUTEX_NR || mutexnr<0)
		usage();

	type = atoi(argv[2]);

	snprintf(filename,_POSIX_PATH_MAX,argv[3]);

	spin_lock(&lock);

	x=0;
	pthread_create(&tid[x], NULL, waitlock, NULL);

	pthread_join(tid[0],NULL);

}
