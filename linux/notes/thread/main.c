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

# define FATAL(format, ...) do {\
     fprintf(stderr, "Fatal %s %d " format,__FILE__, __LINE__, ## __VA_ARGS__);\
     exit(1);\
 }while(0)

unsigned long ctr[2];

#if 0
void *thread(void *arg)
{
	int id = *(int*)arg;

	ctr[id]++;
// OK we could measure the advancememt of the other counter NNOONON
	if(id ==1 )
	while(1){
		fprintf(stderr,"%d",id);
	}
}
#endif

#if 0
void *thread(void *arg)
{
	int id = *(int*)arg;

	while(1){
		fprintf(stderr,"%d",id);
	}
}
#endif

volatile unsigned long long Gx=0,dummy;
void *thread(void *arg)
{
	int id = *(int*)arg;
	
	while(1){
		for(Gx=0;Gx<500*1024*1024;Gx++){
			dummy++;
		}
		fprintf(stderr,".");
	}
	while(1){
		fprintf(stderr,"%d",id);
	}
}

int main (int argc, char*argv[])
{
	pthread_t tid;
	int ida,idb;

	ctr[0]=0;
	ctr[1]=1;
	ida = 0;
	pthread_create(&tid, NULL, thread, &ida);

//	idb = 1;
//	pthread_create(&tid, NULL, thread, &idb);

	while(1) sleep(1);
}

