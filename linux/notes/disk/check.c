#define _XOPEN_SOURCE 600
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#define __USE_GNU
#include <fcntl.h>
#include <strings.h>

/*
 * ./prog filename block_nr
 *
 * The device block size is 512???
 *
 * Create a large file made of multiple block
 * Write those in a loop together with a counter
 */
#define BLOCK_SIZE 512
int main( int argc, char **argv)

{
	void *buf;
	unsigned long *data,new,old;
	char *filename;
	int block_nr;
	int ret = 0;
	unsigned long bytes_written = 0;
	int fd,x;

	if(argc ==3 ){
		filename = argv[1];
		block_nr = atoi(argv[2]);
	}
	else
		return -1;
	
	if( (ret = posix_memalign(&buf, BLOCK_SIZE, BLOCK_SIZE)) ) {
		perror("Memalign failed");
		exit(ret);
	}
	memset(buf, 0, BLOCK_SIZE);

	if( (fd = open(filename, O_RDONLY|O_DIRECT,0666) ) < 0 ) {
		perror("Open failed");
		exit(ret);
	}

	data = buf;
	for(x=0;x<block_nr;x++){

		old = data[1];

		bytes_written = 0;
		while(1){
			ret = pread(fd, buf, BLOCK_SIZE, bytes_written+(x*BLOCK_SIZE));
			if(ret < 0){
				perror("Write failed");
				exit(ret);
			}
			bytes_written += ret;
			if(bytes_written == BLOCK_SIZE)
				break;
		}
		if(data[0]!=0xdeadbeef){
			printf("Inconsistency %x -> %x\n",0xdeadbeef, data[0]);
			exit(-1);
		}
		new = data[1];

		if(new != old+1)
			fprintf(stderr,"Inconsistency %x -> %x\n",new,old);

		if(!(x%(1024*1024/BLOCK_SIZE)))
			fprintf(stderr,".");

	}
	close(fd);

	free(buf);
}



