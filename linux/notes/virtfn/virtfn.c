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

#define DRIVER_VERSION	"0.01"
#define DRIVER_AUTHOR	"Etienne Martineau <etmartin@cisco.com>"
#define DRIVER_DESC	"PCI virtual function"

#define MAX_CFG_SIZE 4096

#define MIN(a,b)        (min(a,b))
#define MAX(a,b)        (max(a,b))

#define VIRTFN_BRIDGE 0
#define VIRTFN_ROOT_PORT 1
#define VIRTFN_ENDPOINT 2

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
	unsigned int bar[PCI_NUM_RESOURCES];

	/* pci config space */
	uint8_t config[MAX_CFG_SIZE];

	/* reference to struct pci_dev */
	struct pci_dev *pdev;
};

extern void client_cleanup(void);
extern int client_init(void);
static int xen_pci_notifier(struct notifier_block *nb,
			    unsigned long action, void *data);

int debug=1;
static LIST_HEAD(vfcn_list);
static DEFINE_SPINLOCK(vfcn_list_lock);
static struct bus_tap bus_tap_lookup[256];

static struct notifier_block device_nb = {                                                                              
    .notifier_call = xen_pci_notifier,
};                                                                                                                      

/*
 * Normally the PCI cfg cycle are decoded at HW level i.e. for a given
 * bus/device/function the endpoint device grabs the transaction.
 * Here this is different since we are emulating the cfg cycle hence we
 * have to 'decode' in SW i.e. walk the list
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
#if 0
[   36.766937] Adjusting nested tapping 15
[   36.767417] Adjusting nested tapping 15
[   36.767879] Adjusting nested tapping 15
[   36.768359] Adjusting nested tapping 15
[   36.768834] vdevice_find: vdevice_find 0 0:9.0
[   36.769337] vfcn_read: CFG Read 18->4= 302500
[   36.769897] vdevice_find: vdevice_find 0 0:9.0
[   36.770402] vfcn_read: CFG Read 3e->2= 0
[   36.770911] vdevice_find: vdevice_find 0 0:9.0
[   36.771415] vfcn_write: CFG Write 3e->2= 0
[   36.772302] BUG: unable to handle kernel NULL pointer dereference at           (null)
[   36.772937] IP: [<          (null)>]           (null)
[   36.772937] PGD 54cb067 PUD 250000000572e067 PMD 5579067 PTE 8000000001d4a225
[   36.772937] Thread overran stack, or stack corrupted
[   36.772937] Oops: 0011 [#1] SMP 
recursive check with that bus tapping crap...
#endif

static int pci_write(struct pci_bus *bus, unsigned int devfn, int where, int size, u32 value)
{
	struct vdev *dev;
	struct bus_tap *tap = &bus_tap_lookup[bus->number];

	dev = vdevice_find(pci_domain_nr(bus), bus->number, devfn);
	if(!dev){
		if(!tap->refcount){
			//printk("Adjusting nested tapping %x\n",bus->number);
			tap = &bus_tap_lookup[0];
			//What is bus tap 0 get us back here???
			if(!(tap->refcount))
				BUG();
		}
		return tap->ops.write(bus,devfn, where, size, value);
	}
	return vfcn_write(dev,where,size,value);
}

static int xen_pci_notifier(struct notifier_block *nb,
			    unsigned long action, void *data)
{
	struct device *dev = data;
	struct pci_dev *pdev = to_pci_dev(dev);

	switch (action) {
	case BUS_NOTIFY_ADD_DEVICE:
		break;
	case BUS_NOTIFY_DEL_DEVICE:
		virtfn_bus_device_del(pci_domain_nr(pdev->bus), pdev->bus->number, 
			pdev->devfn);
		break;
	default:
		return NOTIFY_DONE;
	}
	return NOTIFY_OK;
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
static void tab_bus(struct pci_bus *bus)
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

static void untap_bus(struct pci_bus *bus)
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

/*
 * Akin to pciehp_configure_device that we don't do deepth first scan. Instead
 * we insert device point to point
 *
 * We need to use similar technique than pciehp_configure for deepth first scan
 */
static int virtfn_bus_device_add(int type, unsigned int seg, unsigned int bus,
	unsigned int devfn, uint8_t *conf)
{
	struct vdev *vdev;
	struct pci_bus *b, *c;
	struct pci_dev *pdev;

	/*
	 * First step is to create and customize the virtual device
	 */
	vdev=kzalloc(sizeof(struct vdev) ,GFP_KERNEL);
	if(!vdev)
		return -ENOMEM;
	
	vdev->aseg=0;
	vdev->abus=bus;
	vdev->adevfn=devfn;
	vdev->type = type;
	/* Latch the config locally */
	memcpy(vdev->config, conf, MAX_CFG_SIZE);

	/*
	 * Then we try to find if the bus where we want to insert that 
	 * device exist or not.
	 *
	 * If it exist we 'tap' the bus i.e. we inspect all config RD/WR access
	 * and if they matches to one of the virtual device we use the config space
	 * provided by that virtual device otherwise we redirect to the original
	 * hardware operation
	 */
	b = pci_find_bus(seg, bus);
	if(!b){
		printk("Cannot find bus %d,%d\n",seg ,bus);
		kfree(vdev);
		return -EINVAL;
	}

	pdev = pci_get_slot(b, devfn);
	if(pdev){
		printk("Device %s already exists " 
			"at %04x:%02x:00, cannot hot-add\n", pci_name(pdev),
			pci_domain_nr(b), b->number);
		pci_dev_put(pdev);
		kfree(vdev);
		return -EBUSY;
	}
	
	/*
	 * After this point all the bus->ops RD/WR will be redirected to our
	 * own version. This will be transparent for the existing devices
	 */
	tab_bus(b);

	/* 
	 * Then we add the virtual device on the global list
	 * From that point on PCI can see the new device.
	 * NOTE what happen if a rescan happen after vdevice_add but before
	 * pci_scan_single_device?
	 */
	vdevice_add(vdev);

	/* 
	 * Now populate struct pci_dev{} by scanning the device.
	 * After this step the device is on the pci list of devices 
	 */
	pdev = pci_scan_single_device(b, devfn);
	if(!pdev){
		printk("Cannot discover device %x %x\n",bus, devfn);
		untap_bus(b);
		vdevice_del(vdev);
		kfree(vdev);
		return -EIO;
	}
	vdev->pdev = pci_dev_get(pdev);

	if(type == VIRTFN_ROOT_PORT){
		/* bus are created in /sys/class/pci_bus/ */
		c = pci_add_new_bus(b, pdev, vdev->config[25]);
		if(!c){
			printk("Cannot create new bus %x %x\n",bus, devfn);
			pci_dev_put(vdev->pdev);
			untap_bus(b);
			vdevice_del(vdev);
			kfree(vdev);
			return -ENOMEM;
		}
		c->primary = 0;
		c->subordinate = vdev->config[26];
	}

	/* Insert it into the device tree */
	pci_bus_add_devices(b);

	printk("Adding %s under %x:%x.%x\n",type==VIRTFN_ROOT_PORT ? "root port":"end point",
		vdev->abus,PCI_SLOT(vdev->adevfn), PCI_FUNC(vdev->adevfn));
	return 0;
}

int virtfn_bus_rootport_device_add(unsigned int seg, unsigned int bus,
	unsigned int devfn, uint8_t *conf)
{
	return virtfn_bus_device_add(VIRTFN_ROOT_PORT, seg, bus, devfn, conf);

}

int virtfn_bus_endpoint_device_add(unsigned int seg, unsigned int bus,
	unsigned int devfn, uint8_t *conf)
{
	return virtfn_bus_device_add(VIRTFN_ENDPOINT, seg, bus, devfn, conf);
}

void virtfn_bus_device_del(unsigned int seg, unsigned int bus, 
	unsigned int devfn)
{
	struct pci_bus *b;
	struct vdev *vdev;

	vdev = vdevice_find(seg, bus, devfn);
	if(!vdev)
		return;
	
	b = pci_find_bus(0,vdev->abus);
	if(!b){
		WARN_ON(1);
		printk("DEVICE del Cannot find bus 0,%d\n",vdev->abus);
		return;
	}
	printk("Removing %s under %x:%x.%x\n",vdev->type==VIRTFN_ROOT_PORT ? "root port":"end point",
		vdev->abus,PCI_SLOT(vdev->adevfn), PCI_FUNC(vdev->adevfn));
	pci_dev_put(vdev->pdev);
	untap_bus(b);
	vdevice_del(vdev);
	kfree(vdev);
}

/*
 * Helper function
 */
void destroy_all_virtfn_device(void)
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
		/*
		 * Here we rely on the bus notifier call back to do the cleanup.
		 * This is executed under the same context so we won't miss any entry
		 * from the list...
		 */
		pci_remove_bus_device(vdev->pdev);
		goto again;
	}
}

static int __init init(void)
{
	if(client_init())
		return -1;

	/*
	 * pci_stop_bus_device() will recursively remove all devices under a given
	 * topology. For each devices it will call into device_unregister().
	 * device_unregister() call into the bus->remove method ( for PCI this is
	 * pci_device_remove()). If a driver is bounded to that device it will call
	 * the associated drv->remove method. Then pci_device_remove() will call 
	 * into pci_dev_put() which will free up the pci_dev{} structure.
	 *
	 * The problem here is that pci_dev{} structure is associated with a 
	 * vdev{} structure which cannot be release by normal code path
	 *
	 * For that reason we register for pci bus notifier so that we can track
	 * device removal appropriatly.
	 *
	 * First pci_remove_bus_device() calls into pci_stop_bus_device() and 
	 * then it call into pci_destroy_dev() which remove the device from the 
	 * list and free up the resource
	 */
	bus_register_notifier(&pci_bus_type, &device_nb);

	pr_info(DRIVER_DESC " version: " DRIVER_VERSION);
	return 0;
}

static void __exit cleanup(void)
{
	destroy_all_virtfn_device();
	client_cleanup();
	bus_unregister_notifier(&pci_bus_type, &device_nb);
}

module_init(init);
module_exit(cleanup);

MODULE_VERSION(DRIVER_VERSION);
MODULE_LICENSE("GPL v2");
MODULE_AUTHOR(DRIVER_AUTHOR);
MODULE_DESCRIPTION(DRIVER_DESC);

