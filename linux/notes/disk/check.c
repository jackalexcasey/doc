#define _XOPEN_SOURCE 600

#include <stdio.h>

#include <stdlib.h>

#include <unistd.h>

#include <sys/types.h>

#include <sys/stat.h>

#define __USE_GNU

#include <fcntl.h>

#include <strings.h>


int main( int argc, char **argv)

{
	void *buf;
	char *filename;
	int ret = 0;
	int ps = getpagesize();
	int chunk;
	unsigned long long int bytes_written = 0;
	int fd,y;

	if(argc ==3 ){
		filename = argv[1];
		chunk = atoi(argv[2]);
	}
	else
		return -1;
	
	if( (ret = posix_memalign(&buf, ps, ps*chunk)) ) {
		perror("Memalign failed");
		exit(ret);
	}


	if( (fd = open(filename, O_RDONLY | O_DIRECT) ) < 0 ) {
		perror("Open failed");
		exit(ret);
	}

	for(y=0;y<10;y++){
		bytes_written = 0;
		memset(buf, y, ps*chunk);
		while(1){
			ret = pwrite(fd, buf, ps*chunk, bytes_written);
			if(ret < 0){
				perror("Write failed");
				exit(ret);
			}
			bytes_written += ret;
			if(bytes_written == ps*chunk)
				break;
		}
	}

	printf("Wrote %lld GB\n", bytes_written/1000/1000/1000);

	close(fd);

	free(buf);
}

