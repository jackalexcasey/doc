#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include <signal.h>
#include <errno.h>
#include <sys/eventfd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>

#define DEBUG(args...)  fprintf(stderr,args);

static volatile int ctr=0;
static pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;

void* dummy_thread(void *arg)
{
	pthread_mutex_lock(&lock);
	ctr++;
	pthread_mutex_unlock(&lock);
	pthread_exit(NULL);

}

int main(int argc, char *argv[])
{
	int x;
	pthread_t tid;

	while(1){
		/*
		 * Here we keep creating thread to trigger task_struct slab
		 * activity
		 */
		if(pthread_create(&tid, NULL, dummy_thread,NULL)){
			perror("pthread_create failed");
			return -1;
		}
		if(pthread_join(tid, NULL)){
			perror("pthread_join failed");
			return -1;
		}
	}
}

