#include <stdlib.h>
#include <stdio.h>

volatile unsigned long long Gx=0,dummy;

int main (int argc, char*argv[])
{
	if(argc!=2)
		return;

	while(1){
		for(Gx=0;Gx<(500*1024*1024);Gx++){
			dummy++;
		}
		fprintf(stderr,".");
	}
}

