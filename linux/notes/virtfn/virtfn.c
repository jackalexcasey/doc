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

#define DRIVER_VERSION	"0.01"
#define DRIVER_AUTHOR	"Etienne Martineau <etmartin@cisco.com>"
#define DRIVER_DESC	"PCI virtual function"

#define VDEV_MAX_RESOURCE 6 /* Equivalent to PCI_NUM_RESOURCES */
#define MAX_CFG_SIZE 4096

#define MIN(a,b)        (min(a,b))
#define MAX(a,b)        (max(a,b))

#define VIRTFN_BRIDGE 0
#define VIRTFN_ROOT_PORT 1
#define VIRTFN_ENDPOINT 2

#define DPRINTK(fmt, args...)	\
	do{	\
		if(debug) \
			printk(KERN_DEBUG "%s: " fmt, __func__ , ## args); \
} while (0)

struct bus_tap{
	int refcount;
	struct pci_ops ops;
};

struct vdev{
	struct list_head list;
	
	int type;

	/* PCI topology */
	unsigned int aseg;
	unsigned int abus;
	unsigned int adevfn;

	/* Bridge specific */
	int secondary_bus_nr;
	int subordinate_bus_nr;

	/* bar config */
	unsigned int bar[VDEV_MAX_RESOURCE];

	/* pci config space */
	uint8_t config[MAX_CFG_SIZE];

	/* reference to struct pci_dev */
	struct pci_dev *pdev;
};

static int debug=1;
static LIST_HEAD(vfcn_list);
static DEFINE_SPINLOCK(vfcn_list_lock);
static struct bus_tap bus_tap_lookup[256];
static struct kobject *udrv_kobj;

static inline void vdevice_setbar(struct vdev* dev, unsigned int bar,int size, int type)
{
	type=type;
	if( (bar >= PCI_BASE_ADDRESS_0) && (bar <=PCI_BASE_ADDRESS_5 ) )
		dev->bar[ (bar-PCI_BASE_ADDRESS_0) >>2]=size;
}

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

static void vdevice_add(struct vdev *dev)
{
	spin_lock(&vfcn_list_lock);	
	list_add(&dev->list,&vfcn_list);
	spin_unlock(&vfcn_list_lock);
}

static void vdevice_del(struct vdev *dev)
{
	spin_lock(&vfcn_list_lock);	
	list_del(&dev->list);
	spin_unlock(&vfcn_list_lock);
}

/*
 * Normally the PCI cfg cycle are decoded at HW level i.e. for a given
 * bus/device/function the endpoint device grabs the transaction.
 * Here this is different since we are emulating the cfg cycle hence we
 * have to 'decode' in SW i.e. walk through the list of device on a given bus.
 */
static struct vdev * vdevice_find(unsigned int seg, unsigned int bus,
			  unsigned int devfn)
{
	struct list_head *pos;
	struct vdev *ptr;
	
	spin_lock(&vfcn_list_lock);	
	list_for_each(pos, &vfcn_list){
		ptr= list_entry(pos, struct vdev, list);
		if(seg == ptr->aseg && bus==ptr->abus && (devfn == ptr->adevfn)){
			spin_unlock(&vfcn_list_lock);
			DPRINTK("vdevice_find %x %x:%x.%x",seg,bus,PCI_SLOT(devfn),PCI_FUNC(devfn));
			return ptr;
		}
	}
	spin_unlock(&vfcn_list_lock);
	return NULL;
}

static int vfcn_read(struct vdev *pci_dev, int reg, int len, u32 *val)
{
	u32 value = 0;

	len = MIN(len, (int)(MAX_CFG_SIZE - reg));
	memcpy(&value, (unsigned char*)(pci_dev->config) + reg, len);
	*val = le32_to_cpu(value);
	DPRINTK("CFG Read %x->%d= %x\n",reg,len,*val);
	return 0;
}

static int vfcn_write(struct vdev *pci_dev, int reg, int len, u32 val)
{
	int i;

	DPRINTK("CFG Write %x->%d= %x\n",reg,len,val);
	if(len==4 && ((val &0xffffffff) ==0xfffffffe)){ /* Weird config */
		val=0;
		DPRINTK("BAR %x -> %x\n",reg,val);
	}
	else if(len==4 && ((val &0xfffffff0) ==0xfffffff0)){ /* probing for SIZE */
		val = pci_dev->bar[ (reg-PCI_BASE_ADDRESS_0) >>2];
		DPRINTK("BAR %x -> %x\n",reg,val);
	}
	else if(len==4 && (val == 0xfffff800)){ /* probing for Option rom */
		val = 0;
		DPRINTK("ROM BAR %x -> %x\n",reg,val);
	}
	for (i = 0; i < len; i++) {
		*((unsigned char*)(pci_dev->config) + reg + i) = val & 0xff;
		val >>= 8;
	}
	return 0;
}

static int pci_read(struct pci_bus *bus, unsigned int devfn, int where, int size, u32 *value)
{
	struct vdev *dev;
	struct bus_tap *tap = &bus_tap_lookup[bus->number];
	
	dev = vdevice_find(pci_domain_nr(bus), bus->number, devfn);
	if(!dev){
		if(!tap->refcount){
			//printk("Adjusting nested tapping %x\n",bus->number);
			tap = &bus_tap_lookup[0];
			if(!(tap->refcount))
				BUG();
		}
		return tap->ops.read(bus, devfn, where, size, value);
	}
	return vfcn_read(dev,where,size,value);
}

static int pci_write(struct pci_bus *bus, unsigned int devfn, int where, int size, u32 value)
{
	struct vdev *dev;
	struct bus_tap *tap = &bus_tap_lookup[bus->number];

	dev = vdevice_find(pci_domain_nr(bus), bus->number, devfn);
	if(!dev){
		if(!tap->refcount){
			//printk("Adjusting nested tapping %x\n",bus->number);
			tap = &bus_tap_lookup[0];
			if(!(tap->refcount))
				BUG();
		}
		return tap->ops.write(bus,devfn, where, size, value);
	}
	return vfcn_write(dev,where,size,value);
}

/*
 * In the case when we tap on BUS #0 ( when we create root port for example ),
 * we may endup in pci_read OR pci_write above for busses different than 
 * BUS #0. Reason beeing that generally bus->ops originate from BUS #0.
 *
 * In that case we make an explicit check and redirect the operation to native
 * BUS #0
 * It may be better to walk down all busses and see who has the same ops than
 * our bus* and is they are the same it means we tap it _all_ out..
 *
 *
 * OR if we tap a bus* which is parent of other bus, all the child bus will be
 * redirected automatically because (most of the time) they inherit from the 
 * parent. So when we intercept such a situation we should walk up to the 
 * parent and see if the refcount is value.
 *
 */
void tab_bus(struct pci_bus *bus)
{
	static int init=0;
	struct bus_tap *tap = &bus_tap_lookup[bus->number];

	if(!init){
		memset(bus_tap_lookup, 0, sizeof(bus_tap_lookup));
		init = 1;
	}

	if(tap->refcount){
		tap->refcount++;
		printk("BUS %x already tapped\n",bus->number);
		return;
	}
	
	/* Fallback value */
	tap->ops.read = bus->ops->read;
	tap->ops.write = bus->ops->write;

	/* Our own ops to our own */
	bus->ops->read = pci_read;
	bus->ops->write = pci_write;

	tap->refcount++;
	printk("Tapping BUS %x\n",bus->number);
}

void untap_bus(struct pci_bus *bus)
{
	struct bus_tap *tap = &bus_tap_lookup[bus->number];

	if(tap->refcount){
		if(tap->refcount == 1){
			/* Restore value */
			bus->ops->read = tap->ops.read;
			bus->ops->write = tap->ops.write;
			tap->refcount = 0;
			printk("UN tapping BUS %x\n",bus->number);
		}
		else{
			printk("Decrementing tapping BUS %x\n",bus->number);
			tap->refcount--;
		}
	}
}

void* virtfn_bus_device_add(int type, int busnr, int devfn, int sec, int sub)
{
	struct vdev *vdev;
	struct pci_bus *bus, *child;
	struct pci_dev *pdev;

	/*
	 * First step is to create and customize the virtual device
	 */
	vdev=kzalloc(sizeof(struct vdev) ,GFP_KERNEL);
	if(!vdev)
		return NULL;
	
	vdev->aseg=0;
	vdev->abus=busnr;
	vdev->adevfn=devfn;
	vdev->type = type;

	if(type == VIRTFN_ROOT_PORT){
		memcpy(vdev->config,vbridge_conf,sizeof(vbridge_conf));
		vdev->config[25] = sec;
		vdev->config[26] = sub;
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
	else{
		kfree(vdev);
		return NULL;
	}

	/*
	 * Then we try to find if the bus where we want to insert that 
	 * device exist or not.
	 *
	 * If it exist we 'tap' the bus i.e. we inspect all config RD/WR access
	 * and if they matches to one of the virtual device we use the config space
	 * provided by that virtual device otherwise we redirect to the original
	 * hardware operation
	 */
	bus = pci_find_bus(0,busnr);
	if(!bus){
		printk("Cannot find bus 0,%d\n",busnr);
		kfree(vdev);
		return NULL;
	}
	
	tab_bus(bus);

	/* 
	 * Then we add the virtual device on the global list
	 * From that point on PCI can see the new device
	 */
	vdevice_add(vdev);

	/* Now populate struct pci_dev{} by scanning the device */
	pdev = pci_scan_single_device(bus, devfn);
	if(!pdev){
		printk("Cannot discover device %x %x\n",busnr, devfn);
		untap_bus(bus);
		vdevice_del(vdev);
		kfree(vdev);
		return NULL;
	}
	vdev->pdev = pci_dev_get(pdev);

	if(type == VIRTFN_ROOT_PORT){
		/* bus are created in /sys/class/pci_bus/ */
		child = pci_add_new_bus(bus, pdev, sec);
		if(!child){
			printk("Cannot create new bus %x %x\n",busnr, devfn);
			pci_dev_put(vdev->pdev);
			untap_bus(bus);
			vdevice_del(vdev);
			kfree(vdev);
		}
		child->primary = 0;
		child->subordinate = sub;
	}

	/* Insert it into the device tree */
	pci_bus_add_devices(bus);

	printk("Adding %s under %x:%x.%x\n",type==VIRTFN_ROOT_PORT ? "root port":"end point",
		vdev->abus,PCI_SLOT(vdev->adevfn), PCI_FUNC(vdev->adevfn));
	return vdev;
}
//pci_remove_bus(child);
void virtfn_bus_device_del(struct vdev *vdev)
{
	struct pci_bus *bus;

	bus = pci_find_bus(0,vdev->abus);
	if(!bus){
		WARN_ON(1);
		printk("DEVICE del Cannot find bus 0,%d\n",vdev->abus);
		return;
	}
	printk("Removing %s under %x:%x.%x\n",vdev->type==VIRTFN_ROOT_PORT ? "root port":"end point",
		vdev->abus,PCI_SLOT(vdev->adevfn), PCI_FUNC(vdev->adevfn));
	pci_dev_put(vdev->pdev);
	untap_bus(bus);
	vdevice_del(vdev);
	kfree(vdev);
	return 0;
}

void virtfn_cleanup(void)
{
	struct list_head *pos;
	struct vdev *vdev;

again:
	vdev = NULL;
	spin_lock(&vfcn_list_lock);	
	list_for_each(pos, &vfcn_list){
		vdev = list_entry(pos, struct vdev, list);
		break;
	}
	spin_unlock(&vfcn_list_lock);

	if(vdev){
		pci_stop_and_remove_bus_device(vdev->pdev);
		goto again;
	}
}

static int xen_pci_notifier(struct notifier_block *nb,
			    unsigned long action, void *data)
{
	struct device *dev = data;
	struct pci_dev *pdev = to_pci_dev(dev);
	struct vdev *vdev;
	int r = 0;

	switch (action) {
	case BUS_NOTIFY_ADD_DEVICE:
		break;
	case BUS_NOTIFY_DEL_DEVICE:
		/*
		 * Are we dealing with a virtual device or not
		 */
		vdev = vdevice_find(pci_domain_nr(pdev->bus), pdev->bus->number, pdev->devfn);
		if(vdev)
			virtfn_bus_device_del(vdev);
		break;
	default:
		return NOTIFY_DONE;
	}
	return NOTIFY_OK;
}

static struct notifier_block device_nb = {                                                                              
    .notifier_call = xen_pci_notifier,
};                                                                                                                      

/*
 * Helper function
 */
void* create_root_port(int devfn, int sec, int sub)
{
	return virtfn_bus_device_add(VIRTFN_ROOT_PORT,0,devfn,sec,sub);
}

/*
 * Helper function
 */
void* create_device(int busnr, int devfn)
{
	return virtfn_bus_device_add(VIRTFN_ENDPOINT,busnr,devfn,0,0);
}


static ssize_t debug_show(struct kobject *kobj, struct kobj_attribute *attr,
			char *buf)
{
	virtfn_cleanup();
	return 0;
}

static ssize_t debug_store(struct kobject *kobj, struct kobj_attribute *attr,
			 const char *buf, size_t count)
{
	void *rootport, *device;

	/* Create a root port at 0:9.0 and subordinate bus is 25 */
	rootport = create_root_port(PCI_DEVFN(9,0), 0x25, 0x30);
	if(!rootport)
		return -EINVAL;
	/* Create a device at 25:5.0 */
	device = create_device(0x25, PCI_DEVFN(5,0));
	if(!device)
		return -EINVAL;
	/* Create a device at 25:6.0 */
	device = create_device(0x25, PCI_DEVFN(6,0));
	if(!device)
		return -EINVAL;
	/* Create a device at 25:7.0 */
	device = create_device(0x25, PCI_DEVFN(7,0));
	if(!device)
		return -EINVAL;

	/* Create a root port at 0:10.0 and subordinate bus is 35 */
	rootport = create_root_port(PCI_DEVFN(0x10,0), 0x35, 0x40);
	if(!rootport)
		return -EINVAL;
	/* Create a device at 35:5.0 */
	device = create_device(0x35, PCI_DEVFN(5,0));
	if(!device)
		return -EINVAL;
	/* Create a device at 35:5.0 */
	device = create_device(0x35, PCI_DEVFN(1,0));
	if(!device)
		return -EINVAL;
	/* Create a device at 35:5.0 */
	device = create_device(0x35, PCI_DEVFN(2,0));
	if(!device)
		return -EINVAL;
	/* Create a device at 35:5.0 */
	device = create_device(0x35, PCI_DEVFN(3,0));
	if(!device)
		return -EINVAL;
	return count;
}

static struct kobj_attribute debug_attribute =
	__ATTR(debug, 0666, debug_show, debug_store);

static struct attribute *attrs[] = {
	&debug_attribute.attr,
	NULL,	/* need to NULL terminate the list of attributes */
};

static struct attribute_group attr_group = {
	.attrs = attrs,
};

static int __init init(void)
{
	int retval;

	udrv_kobj = kobject_create_and_add("virtfn", kernel_kobj);
	if (!udrv_kobj)
		return -ENOMEM;

	/* Create the files associated with this kobject */
	retval = sysfs_create_group(udrv_kobj, &attr_group);
	if (retval)
		kobject_put(udrv_kobj);

	/*

	 * Device removal works this way;
	 * The device is removed using pci_stop_bus_device(struct pci_dev *dev);
	 * 	This function will recursively remove all device under the topology.
	 * 	Then it will call into pci_stop_dev() which will release the sysfs/ files
	 * 	and call device_unregister().
	 * 	  device_unregister() will trigger a pci_device_remove() ( which if a driver
	 * 	  is bounded to it will call drv->remove ) then pci_device_remove() will call
	 * 	  into pci_dev_put() to free up the pci_dev{} structure
	 *
	 * The problem with this scheme is that struct vdev{} is never released.
	 *
	 * Moreover the associated bus is not taken out of tap mode either.
	 *
	 * For that reason we register for bus notifier on the pci_bus so that
	 * we can track device removal appropriatly.
	 */
	bus_register_notifier(&pci_bus_type, &device_nb);

	pr_info(DRIVER_DESC " version: " DRIVER_VERSION);
	return retval;
}

static void __exit cleanup(void)
{
	virtfn_cleanup();
	kobject_put(udrv_kobj);
	bus_unregister_notifier(&pci_bus_type, &device_nb);
}

module_init(init);
module_exit(cleanup);

MODULE_VERSION(DRIVER_VERSION);
MODULE_LICENSE("GPL v2");
MODULE_AUTHOR(DRIVER_AUTHOR);
MODULE_DESCRIPTION(DRIVER_DESC);

