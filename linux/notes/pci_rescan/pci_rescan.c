#include <linux/module.h>
#include <linux/device.h>
#include <linux/err.h>
#include <linux/kdev_t.h>
#include <linux/idr.h>
#include <linux/hwmon.h>
#include <linux/gfp.h>
#include <linux/spinlock.h>
#include <linux/pci.h>

#include <linux/kallsyms.h>
#include <linux/kexec.h>
#include <linux/sysfs.h>
#include <linux/bug.h>

static int b=-1;
MODULE_PARM_DESC(b, "");
module_param(b, int, 0644);

static int d=-1;
MODULE_PARM_DESC(d, "");
module_param(d, int, 0644);

static int f=-1;
MODULE_PARM_DESC(f, "");
module_param(f, int, 0644);

static int debug=1;

#define DPRINTK(fmt, args...)	\
	do{	\
		if(debug) \
			printk(KERN_DEBUG "%s: " fmt, __func__ , ## args); \
} while (0)

static struct pci_dev *Gpdev;
static struct pci_ops local_pci_root_ops;

static int pci_read(struct pci_bus *bus, unsigned int devfn, int reg, int len, u32 *val)
{
	int value;
	//struct task_struct *cur = current;
//	if(!strcmp(cur->comm,"bash"))
	
	value = local_pci_root_ops.read(bus, devfn, reg, len, val);
//	DPRINTK("%x/%lx %x:%x.%x\n",reg,value,bus->number, PCI_SLOT(devfn), PCI_FUNC(devfn));
	return value;
}

static int pci_write(struct pci_bus *bus, unsigned int devfn, int reg, int len, u32 val)
{
	DPRINTK("%x/%lx %x:%x.%x\n",reg,val,bus->number, PCI_SLOT(devfn), PCI_FUNC(devfn));
	return local_pci_root_ops.write(bus,devfn, reg, len, val);
}

static int latch_to_pci_bus(void)
{
	/*
	 * The first thing we do is to override the rd/wr pci_cfg ops
	 */
	Gpdev = pci_get_bus_and_slot(b,PCI_DEVFN(d,f));
	if (!Gpdev){
		DPRINTK("Cannot find %x:%x.%x ",b,d,f);
		return -EINVAL;
	}

	DPRINTK("Found %x:%x.%x \n",b,d,f);
	pci_block_user_cfg_access(Gpdev);

	/* Fallback value */
	local_pci_root_ops.read = Gpdev->bus->ops->read;
	local_pci_root_ops.write = Gpdev->bus->ops->write;

	/* Redirect ops to our own  HERE we redirect the whole BUS rd/wr...*/
	Gpdev->bus->ops->read = pci_read;
	Gpdev->bus->ops->write = pci_write;

	pci_unblock_user_cfg_access(Gpdev);

	return 0;
}

static void unlatch_from_pci_bus(void)
{
	pci_block_user_cfg_access(Gpdev);

	/* Restore ops */
	Gpdev->bus->ops->read = local_pci_root_ops.read;
	Gpdev->bus->ops->write = local_pci_root_ops.write;

	pci_unblock_user_cfg_access(Gpdev);

	Gpdev = NULL;
}
#if 0
int test_rescan_bus(struct pci_bus *bus)
{
	struct pci_dev *pdev;

	printk("PCI: Scanning bus %04x:%02x\n", pci_domain_nr(bus), bus->number);

	/* Find the target device / downstream port */
	pdev = pci_get_bus_and_slot(b,PCI_DEVFN(d,f));
	if (!pdev){
		DPRINTK("Cannot find %x:%x.%x ",b,d,f);
		return -EINVAL;
	}
	DPRINTK("Found %x:%x.%x \n",b,d,f);
	test_rescan_bus(pdev->bus);
}
#endif

static int __init hwmon_init(void)
{
	return latch_to_pci_bus();
}

static void __exit hwmon_exit(void)
{
	unlatch_from_pci_bus();
}

module_init(hwmon_init);
module_exit(hwmon_exit);


MODULE_AUTHOR("Mark M. Hoffman <mhoffman@lightlink.com>");
MODULE_DESCRIPTION("hardware monitoring sysfs/class support");
MODULE_LICENSE("GPL");

#if 0
void test_rescan_bus(struct pci_bus *bus)
{
    unsigned int max;
    struct pci_dev *dev;

	printk("PCI: Scanning bus %04x:%02x\n", pci_domain_nr(bus), bus->number)

    list_for_each_entry(dev, &bus->devices, bus_list){
		printk("%x -- %x\n",dev->device,dev->vendor);
	}
}
	struct pci_dev *pdev;

	/* Find the target device / downstream port */
	pdev = pci_get_bus_and_slot(b,PCI_DEVFN(d,f));
	if (!pdev){
		DPRINTK("Cannot find %x:%x.%x ",b,d,f);
		return -EINVAL;
	}
	DPRINTK("Found %x:%x.%x \n",b,d,f);
	//if(pdev->bus->self)
	//	DPRINTK("Under %x -- %x",pdev->bus->self->device, pdev->bus->self->vendor);
//	test_rescan_bus(pdev->bus);
#endif


