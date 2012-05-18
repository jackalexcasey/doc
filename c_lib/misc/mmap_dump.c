/*
 * Copyright (C) 2010 Cisco Systems
 *
 * Author: Etienne Martineau <etmartin@cisco.com>
 * 
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
*/
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

void error(void)
{
	perror("");
	exit(-1);
}
int main (int argc, char*argv[])
{
	int fd, x;
	unsigned char *map;

	if(argc != 3)
		error();

	fd =open(argv[1], O_RDWR);
	if(fd<0)
		error();
	map = mmap(NULL, atoi(argv[2]), PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0);
	if(map == MAP_FAILED)
		error();
	
	printf("\n");
	for(x=0;x<atoi(argv[2]);x++){
		if(!(x%4))
			printf("  ");
		if(!(x%32))
			printf("\n");
		printf("%x_",map[x]);
	}
	printf("\n");
}
