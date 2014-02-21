#ifndef __VIRTFN_H__
#define __VIRTFN_H__

#define DPRINTK(fmt, args...)	\
	do{	\
		if(debug) \
			printk(KERN_DEBUG "%s: " fmt, __func__ , ## args); \
} while (0)

extern int debug;

int virtfn_bus_rootport_device_add(unsigned int seg, unsigned int bus,
	unsigned int devfn, uint8_t *conf);

int virtfn_bus_endpoint_device_add(unsigned int seg, unsigned int bus,
	unsigned int devfn, uint8_t *conf);

void virtfn_bus_device_del(unsigned int seg, unsigned int bus, 
	unsigned int devfn);
void destroy_all_virtfn_device(void);

#endif
