#include <stdlib.h>
#include <string.h>
#include <stdio.h>

/* need that for the macro... */
#define PAGE_SIZE 4096

#define __ALIGN_MASK(x,mask)    (((x)+(mask))&~(mask))
#define ALIGN(x,a)      __ALIGN_MASK(x,(typeof(x))(a)-1)
#define PAGE_ALIGN(addr) ALIGN(addr, PAGE_SIZE)
/*
 * From /proc/meminfo
 * MemFree:         2617084 kB
 *
 * We could also use memfree from proc/meminfo and feed it in here
 * 	The effect would be to fall below the acceptable treshold of
 * 	kswapd and start evicting pages
 * 	After every iteration the MemFree will increase because kswapd free only chunk
 * 	at a time until the high water mark is reached again.
 *
 */

struct pages{
	unsigned long *page;
};

/*
 * Need to find the limit where kswapd will not have any room to breadth
 * One way is when chewmem has all the CPU is means kswapd doesn't run
 *
 * Another way would be to use a convergence algo when idle ( the memory needed to
 * survived )
 *
 */
int main(int argc, char **argv)
{
	int i;
	char cmd[256];
	struct pages *page_array;
	unsigned long long page_nr,size,kb,page_ctrl_nr;

	if(PAGE_SIZE != getpagesize())
		return -1;
	
	if (argc < 2)
		return 1;
	kb = atoi(argv[1]);
	if (kb <= 0)
		return -1;
	
	page_nr = PAGE_ALIGN(kb*1024) / getpagesize();
	page_ctrl_nr = (PAGE_ALIGN(page_nr*sizeof(struct pages)))/getpagesize();

	page_array = malloc(page_ctrl_nr*getpagesize());
	if(!page_array)
		return -1;

	page_nr = page_nr - page_ctrl_nr;
	if(page_nr ==0)
		return -1;

	size = (page_ctrl_nr + page_nr) * getpagesize();

	fprintf(stderr,"Req size=%LdkB; Ctrl pages=%Ld; pages=%Ld; Total size %LdkB \n",
		kb, page_ctrl_nr, page_nr, (size/1024));

	while(1){
		for (i=0; i<page_nr; i++) {
			page_array[i].page = malloc(getpagesize());
			if(!page_array[i].page)
				return -1;
			page_array[i].page[0] = 1;
		}
		fprintf(stderr,"Allocation completed\n");
		sprintf(cmd,"cat /proc/meminfo  |grep -e \"MemTotal\" -e \"MemFree\"\n");
		system(cmd);
		sleep(1);
		for (i=0; i<page_nr; i++) {
			free(page_array[i].page);
		}
		fprintf(stderr,"Freeing up DONE\n");
	}
}


