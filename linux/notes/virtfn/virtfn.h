#ifndef __VIRTFN_H__
#define __VIRTFN_H__

int virtfn_bus_device_add(int type, int busnr, int devfn, int sec, int sub);
void virtfn_bus_device_del(unsigned int seg, unsigned int bus, 
	unsigned int devfn);
int create_root_port(int devfn, int sec, int sub);
int create_device(int busnr, int devfn);
void destroy_all_virtfn_device(void);

#endif
