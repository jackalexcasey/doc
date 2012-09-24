#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/mman.h>
       #include <sys/types.h>
	          #include <sys/stat.h>
			         #include <fcntl.h>

#define PCI_SYSFS "/sys/bus/pci/"

typedef struct pci{
	int fd;
	char name[256];
	char path[256];
	unsigned domain;
	unsigned bus;
	unsigned slot;
	unsigned function;
}pciDevice;

volatile unsigned char Global;

pciDevice *
pciGetDevice(unsigned domain,
             unsigned bus,
             unsigned slot,
             unsigned function)
{
    pciDevice *dev;
    char *vendor, *product;
	int x,y,fd;
	unsigned char *ptr=NULL;
	dev = malloc(sizeof(pciDevice));
	if (!dev)
		return NULL;
	
    dev->fd       = -1;
    dev->domain   = domain;
    dev->bus      = bus;
    dev->slot     = slot;
    dev->function = function;

    snprintf(dev->name, sizeof(dev->name), "%.4x:%.2x:%.2x.%.1x",
             dev->domain, dev->bus, dev->slot, dev->function);
    //snprintf(dev->path, sizeof(dev->path),
      //       PCI_SYSFS "devices/%s/config", dev->name);
    snprintf(dev->path, sizeof(dev->path),
             PCI_SYSFS "devices/%s/resource0", dev->name);

	x=0;
	y=0;
	while(1){
		if (access(dev->path, F_OK) != 0) {
			y++;
			if(ptr)
				Global = ptr[0];
			if(!(y%(1024*1024)))
				fprintf(stderr,"#");
			continue;

//			fprintf(stderr, "Device %s not found: could not access %s", dev->name, dev->path);
//			free(dev);
			return NULL;
		}
		if(!x){
			fd = open(dev->path,O_RDWR);
			if(fd<0){
				fprintf(stderr," CANNOT OPEN %s", dev->name, dev->path);
				return NULL;
			}
			ptr == mmap(NULL,1024,PROT_READ|PROT_WRITE,MAP_SHARED,fd,0);
			if(ptr == MAP_FAILED){
				fprintf(stderr," CANNOT MMAP %s", dev->name, dev->path);
				return NULL;
			}
			fprintf(stderr, "Device %s Found %s", dev->name, dev->path);

		}

		x++;
		if(ptr)
			Global = ptr[0];
		if(!(x%(1024*1024)))
			fprintf(stderr,".");
	}

    return dev;
}


int main(int argc, char * argv[])
{
	if(argc != 4)
		return -1;
	pciGetDevice(0,atoi(argv[1]),atoi(argv[2]),atoi(argv[3]));
	return 0;
}


