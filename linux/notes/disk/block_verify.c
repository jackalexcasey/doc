#define _XOPEN_SOURCE 600
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#define __USE_GNU
#include <fcntl.h>
#include <strings.h>

static char *myname;    /* Name that we were invoked under */

/*
 * ./prog filename block_nr
 *
 * The device block size is 512???
 *
 * Create a large file made of multiple block
 * Write those in a loop together with a counter
 */
void usage(void)
{
	fprintf(stderr,"%s -f filename -b block nr [-d] O_DIRECT mode [-B] block size (default 512)\n",myname);
	exit(1);
}


/*
 * ./prog filename block_nr
 *
 * The device block size is 512???
 *
 * Create a large file made of multiple block
 * Write those in a loop together with a counter
 */
int main( int argc, char **argv)

{
	int opt;
	void *buf;
	unsigned long *data,new,old;
	char *filename=NULL;
	int block_nr=0;
	int ret = 0;
	unsigned long bytes_written = 0;
	int fd,x;
	int flags;
	int BLOCK_SIZE=512;


	myname = argv[0];

	while ((opt = getopt(argc, argv, "f:b:dB:")) != -1) {
		switch (opt) {
			case 'f':
				filename = optarg;
				break;
			case 'b':
				block_nr = atoi(optarg);
				break;
			case 'B':
				BLOCK_SIZE= atoi(optarg);
				break;
			case 'd':
				flags = flags | O_DIRECT;
				break;
			default:
				usage();
		}
	}

	if(!filename || block_nr==0)
		usage();
	
	argc -= optind;
	argv += optind;

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



