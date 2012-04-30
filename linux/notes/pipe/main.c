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


int file_read(int fd, void *buf, int size)
{
	int rc;
	while(size>0){
	    rc = read(fd,buf,size);
		if(rc<0){
			if(errno != EINTR)
				return -1;
		}
		else if(rc==0)
			return 0;
		else{
			buf += rc;
			size -= rc;
		}
	}
	return 1;
}

#define BUF_SIZE 0x8000
int main (int argc, char*argv[])
{
	int x;
	int rc;
	int fd;
	FILE* mem;
	unsigned char buf[BUF_SIZE];
	char * line = NULL;
	size_t len = 0;
	ssize_t read;
	char out[256];
	int val;
	char *ptr;

	if(argc<2)
		return -1;
	
	fd = open(argv[1],O_RDWR|O_CREAT);
	if(fd<0)
		return -1;

	do{
		memset(buf,0,sizeof(buf));
		rc = file_read(fileno(stdin),buf,BUF_SIZE);
		if(rc == 0){
			fprintf(stderr,"DONE\n");
			return -1;
		}
		if(rc <0){
			fprintf(stderr,"ERROR");
			return -1;
		}

		mem = fopen("/proc/meminfo","r");
		if(mem == NULL)
			return -1;

		for(x=0;x<4;x++){
			line = NULL;
			if(getline(&line, &len, mem)<0) {
				perror("");
				return -1;
			}

			if(x==1 || x==3){
				ptr = strstr(line,": ");
				ptr++;
				memset(out,0,256);
				sscanf(ptr,"%d",&val);
				sprintf(out,"%d,",val);
//				fprintf(stderr,"%d\n",val);
				write(fd,out,strlen(out));
			}

			if (line)
				free(line);
		}
		fclose(mem);
		write(fd,"\n",strlen("\n"));
		fprintf(stderr,".");
	}while(rc);
	close(fd);
}


