#ifndef __VIRTFN_H__
#define __VIRTFN_H__

#define DPRINTK(fmt, args...)	\
	do{	\
		if(debug) \
			printk(KERN_DEBUG "%s: " fmt, __func__ , ## args); \
} while (0)

extern int debug;

#define VIRTFN_BRIDGE 0
#define VIRTFN_ROOT_PORT 1
#define VIRTFN_ENDPOINT 2

int virtfn_bus_device_add(int type, int busnr, int devfn, uint8_t *conf);
void virtfn_bus_device_del(unsigned int seg, unsigned int bus, 
	unsigned int devfn);
int create_root_port(int devfn, int sec, int sub);
int create_device(int busnr, int devfn);
void destroy_all_virtfn_device(void);

#endif
