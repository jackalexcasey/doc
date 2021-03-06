#include <linux/device.h>
#include <linux/module.h>
#include <linux/eventfd.h>
#include <linux/sched.h>
#include <linux/pci.h>
#include <linux/semaphore.h>
#include <linux/kfifo.h> 
#include <linux/poll.h>
#include <linux/spinlock.h>
#include <linux/bitops.h>
#include <linux/interrupt.h>
#include <linux/uio_driver.h>
#include <linux/platform_device.h>
#include <asm/uaccess.h>
#include <asm/ioctls.h>
#include "virtfn.h"

/* This is a msix virtual device template */
static uint8_t msix_conf[] = {
	0x37, 0x11, 0x00, 0x23, 0x03, 0x00, 0x10, 0x00, 0x10, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x40, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x11, 0x4c, 0xfe, 0x00, 0x02, 0x00, 0x00, 0x00, 0x02, 0x08, 0x00, 0x00, 0x10, 0x00, 0x02, 0x00,
	0x00, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x11, 0x04, 0x00, 0x00, 0x00, 0x00, 0x11, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x30, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
};

/* This is a root port template ( bridge ) */
static uint8_t vbridge_conf[] = {
	0x86, 0x80, 0x20, 0x34, 0x03, 0x00, 0xff, 0xff, 0x10, 0x00, 0x04, 0x06, 0x00, 0x00, 0x01, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x40, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x10, 0x00, 0x42, 0x00, 0x00, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x11, 0x04, 0x00, 0x00,
	0x00, 0x00, 0x11, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x30, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};
#if 0
/*
 * Helper function
 */
static inline void vdevice_setbar(struct vdev* dev, unsigned int bar,int size, int type)
{
	type=type;
	if( (bar >= PCI_BASE_ADDRESS_0) && (bar <=PCI_BASE_ADDRESS_5 ) )
		dev->bar[ (bar-PCI_BASE_ADDRESS_0) >>2]=size;
}
	else if(type == VIRTFN_ENDPOINT){
		memcpy(vdev->config, msix_conf,sizeof(msix_conf));
		vdevice_setbar(vdev, PCI_BASE_ADDRESS_0, 0xfffff000, 0);
		vdevice_setbar(vdev, PCI_BASE_ADDRESS_1, 0x0, 0);
		vdevice_setbar(vdev, PCI_BASE_ADDRESS_2, 0xfffff000, 0);
		vdevice_setbar(vdev, PCI_BASE_ADDRESS_3, 0x0, 0);
		vdevice_setbar(vdev, PCI_BASE_ADDRESS_4, 0x0, 0);
		vdevice_setbar(vdev, PCI_BASE_ADDRESS_5, 0x0, 0);
	}

#endif

static ssize_t debug_show(struct kobject *kobj, struct kobj_attribute *attr,
			char *buf);
static ssize_t debug_store(struct kobject *kobj, struct kobj_attribute *attr,
			 const char *buf, size_t count);

static struct kobj_attribute debug_attribute =
	__ATTR(debug, 0666, debug_show, debug_store);

static struct attribute *attrs[] = {
	&debug_attribute.attr,
	NULL,	/* need to NULL terminate the list of attributes */
};

static struct attribute_group attr_group = {
	.attrs = attrs,
};

static struct kobject *udrv_kobj;

/*
 * Helper function
 */
int create_root_port(int devfn, int sec, int sub)
{
	vbridge_conf[25] = sec;
	vbridge_conf[26] = sub;
	return virtfn_bus_rootport_device_add(0, 0, devfn, vbridge_conf);
}

/*
 * Helper function
 */
int create_device(int busnr, int devfn)
{
	return virtfn_bus_endpoint_device_add(0, busnr, devfn, msix_conf);
}


static ssize_t debug_show(struct kobject *kobj, struct kobj_attribute *attr,
			char *buf)
{
	destroy_all_virtfn_device();
	return 0;
}

static ssize_t debug_store(struct kobject *kobj, struct kobj_attribute *attr,
			 const char *buf, size_t count)
{
	int rootport, device;

	/* Create a root port at 0:9.0 and subordinate bus is 25 */
	rootport = create_root_port(PCI_DEVFN(9,0), 0x25, 0x30);
	if(rootport)
		return -EINVAL;
	/* Create a device at 25:5.0 */
	device = create_device(0x25, PCI_DEVFN(5,0));
	if(device)
		return -EINVAL;
	/* Create a device at 25:6.0 */
	device = create_device(0x25, PCI_DEVFN(6,0));
	if(device)
		return -EINVAL;
	/* Create a device at 25:7.0 */
	device = create_device(0x25, PCI_DEVFN(7,0));
	if(device)
		return -EINVAL;

	/* Create a root port at 0:10.0 and subordinate bus is 35 */
	rootport = create_root_port(PCI_DEVFN(0x10,0), 0x35, 0x40);
	if(rootport)
		return -EINVAL;
	/* Create a device at 35:5.0 */
	device = create_device(0x35, PCI_DEVFN(5,0));
	if(device)
		return -EINVAL;
	/* Create a device at 35:5.0 */
	device = create_device(0x35, PCI_DEVFN(1,0));
	if(device)
		return -EINVAL;
	/* Create a device at 35:5.0 */
	device = create_device(0x35, PCI_DEVFN(2,0));
	if(device)
		return -EINVAL;
	/* Create a device at 35:5.0 */
	device = create_device(0x35, PCI_DEVFN(3,0));
	if(device)
		return -EINVAL;
	return count;
}

int client_init(void)
{
	int retval;

	udrv_kobj = kobject_create_and_add("virtfn", kernel_kobj);
	if (!udrv_kobj)
		return -ENOMEM;

	/* Create the files associated with this kobject */
	retval = sysfs_create_group(udrv_kobj, &attr_group);
	if (retval){
		kobject_put(udrv_kobj);
		return retval;
	}
	return 0;
}

void client_cleanup(void)
{
	kobject_put(udrv_kobj);
}

