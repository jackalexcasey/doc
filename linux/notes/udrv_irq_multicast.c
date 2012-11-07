/*
 * Copyright (C) 2010 Cisco Systems
 * Author: Etienne Martineau <etmartin@cisco.com>
 *
 * This work is licensed under the terms of the GNU GPL, version 2.
 *
 */

#include <linux/uio_pci_proxy.h>

/*
 * There is 2 level of lists:
 *
 * The global IRQ multicast list is a per IRQ slot list of descriptor
 * This list contains context from potentially multiple process.
 *
 * The per FD list is a list of pointer to descriptor present in the 
 * Global IRQ list. This list is usefull at clean-up time when an
 * FD is released where we need to remove it's entry from the global list
 */
#if 0
struct irq_fd_multicast{
	struct irq_descriptor *desc;
	struct list_head list;
};
#endif

struct irq_multicast{
	struct irq_descriptor desc;
	struct task_struct *rtn;
	struct list_head list;
};

struct irq_mc_entry{
	struct list_head list;
	atomic_t refcount;
};

static struct irq_mc_entry irq_mc[256];
static DEFINE_MUTEX(irq_mc_mutex);
static DEFINE_SPINLOCK(irq_mc_lock);

static irqreturn_t udrv_mc_msi(int irq, void *dev_id)
{
	struct irq_multicast *mc;
	struct irq_descriptor *desc;
	struct irq_mc_entry *entry = dev_id;

	if(!atomic_read(&entry->refcount)){
		UDRV_DPRINTK("Spurious multicast IRQ %d\n",irq);
		return IRQ_HANDLED;
	}

	/*
	 * Traverse the list and signal all eventfd on it
	 */
	spin_lock_irq(&irq_mc_lock);
	list_for_each_entry(mc, &entry->list, list){
		desc = &mc->desc;
		eventfd_signal(desc->irq_eventfd, 1);
	}
	spin_unlock_irq(&irq_mc_lock);

	UDRV_DPRINTK("MC irq %d",irq);
	return IRQ_HANDLED;
}

static int udrv_register_irq_mc(int irq, int efd, struct uio_info *dev_info)
{
	int rc;
	struct irq_multicast *mc;
	struct irq_descriptor *desc;

	/* Basic sanity check */
	if(irq > 255 || irq < 0)
		return -EINVAL;
	
	mc = kzalloc(sizeof(struct irq_multicast), GFP_KERNEL);
	if(!mc)
		return -ENOMEM;

	INIT_LIST_HEAD(&mc->list);

	/* Keeping track of who's registering what */
	mc->rtn = current;

	desc = &mc->desc;
	desc->irq = irq;
	desc->flags = 0; /* interrupt is enabled to begin with */
	desc->irq_eventfd = eventfd_ctx_fdget(efd);

	if (IS_ERR(desc->irq_eventfd)){
		rc = PTR_ERR(desc->irq_eventfd);
		kfree(mc);
		return rc;
	}

	/* Serialize this operation */
	mutex_lock(&irq_mc_mutex);

	/*
	 * If there is already an irq_multicast entry on this slot
	 * all we need to do is to add the current descriptor and return
	 */
	if(atomic_read(&irq_mc[irq].refcount)){

		spin_lock_irq(&irq_mc_lock);
		atomic_inc(&irq_mc[irq].refcount);
		list_add(&mc->list, &irq_mc[irq].list);
		spin_unlock_irq(&irq_mc_lock);

		mutex_unlock(&irq_mc_mutex);
		UDRV_DPRINTK("Adding element to existing irq multicast group %d\n",irq);
		return 0;
	}

	/*
	 * Here, there is no irq_multicast entry on this slot BUT we don't know 
	 * if this slot is already taken by a standard IRQ or not.
	 *
	 * If request_irq fails it means that it's already taken by std IRQ.
	 * Simply return EALREADY
	 *
	 * If it pass, we don't want to give an invalid entry ( &irq_mc[irq] ) 
	 * to the IRQ handler so we add it just in case and clean-up if we where
	 * wrong. Since the mc IRQ handler is not active we can speculate on the
	 * entry
	 */
	spin_lock_irq(&irq_mc_lock);
	atomic_inc(&irq_mc[irq].refcount);
	list_add(&mc->list, &irq_mc[irq].list);
	spin_unlock_irq(&irq_mc_lock);

	/*
	 * We could use shared IRQ where we request_irq (SHARED ) for all MC entry
	 * This could simplify the free logic
	 */
	rc = request_irq(irq, udrv_mc_msi, 0, dev_info->name, &irq_mc[irq]);
	if(rc){
		UDRV_DPRINTK("Cannot add IRQ %d to multicast group\n",irq);

		/* Remove the speculative entry */
		spin_lock_irq(&irq_mc_lock);
		atomic_dec(&irq_mc[irq].refcount);
		list_del(&mc->list);
		spin_unlock_irq(&irq_mc_lock);

		mutex_unlock(&irq_mc_mutex);
		eventfd_ctx_put(desc->irq_eventfd);
		kfree(mc);
		return -EALREADY;
	}

	mutex_unlock(&irq_mc_mutex);
	UDRV_DPRINTK("Creating new multicast group %d\n",irq);
	return 0;
}

static int udrv_release_irq_mc(int irq, struct uio_info *dev_info)
{
	struct irq_multicast *mc,*n, *t;
	struct irq_descriptor *desc;
	
	if(irq > 255 || irq < 0)
		return -EINVAL;

	if(!atomic_read(&irq_mc[irq].refcount)){
		UDRV_DPRINTK("IRQ %d not in a multicast group\n",irq);
		return -EINVAL;
	}

again:
	t = NULL;
	spin_lock_irq(&irq_mc_lock);
	list_for_each_entry_safe(mc, n, &irq_mc[irq].list, list){
		if(same_thread_group(mc->rtn, current) ){
			t = mc;
			list_del(&mc->list);
			atomic_dec(&irq_mc[irq].refcount);
			break;
		}
	}
	spin_unlock_irq(&irq_mc_lock);

	if(t){
		/* When it reach 0 free the IRQ slot all together */
		if(!atomic_read(&irq_mc[irq].refcount)){
			free_irq(t->desc.irq,&t->desc);
			UDRV_DPRINTK("IRQ %d exit from multicast group\n",irq);
		}
		eventfd_ctx_put(t->desc.irq_eventfd);
		kfree(t);
		goto again;
	}
	UDRV_DPRINTK("");
	return 0;
}

static int ioctl (struct uio_info *info,
	unsigned int cmd, unsigned long args)
{
	int rc;
	union pci_proxy_ioctl ioctl;
	int eventfd,irq_vector;
	
	if (copy_from_user(&ioctl, (unsigned long __user *) args,sizeof(union pci_proxy_ioctl))){
		UDRV_DPRINTK("");
		return -EFAULT;
	}
	switch (cmd) {
		case PCI_PROXY_IRQ_MULTICAST_REGISTER:
			eventfd=ioctl.irq.efd;
			irq_vector=ioctl.irq.irq;
			rc = udrv_register_irq_mc(irq_vector, eventfd, info);
			return rc;
		case PCI_PROXY_IRQ_MULTICAST_UNREGISTER:
			irq_vector=ioctl.irq.irq;
			rc = udrv_release_irq_mc(irq_vector, info);
			return rc;
		default:
			return -EINVAL;
	}
}

static int release(struct uio_info *info, struct inode *inode)
{
	int x;

	for(x=0;x<256;x++){
		udrv_release_irq_mc(x, info);
	}

	UDRV_DPRINTK("");
	return 0;
}

static int probe(struct platform_device *pdev)
{
	int ret = -ENODEV;
	struct uio_info *info;
	
	info = kzalloc(sizeof(struct uio_info), GFP_KERNEL);
	if (!info){
		ret = -ENOMEM;
		goto bad;
	}

	info->name = UIO_PCI_PROXY_IRQ_MULTICAST;
	info->version = DRIVER_VERSION;
	info->ioctl = ioctl;
	info->irq = UIO_IRQ_CUSTOM;

	info->release = release;

	platform_set_drvdata(pdev, info);

	ret = uio_register_device(&pdev->dev, info);
	if(ret){
		printk(KERN_ERR " %s cannot register with UIO",info->name);
		goto bad1;
	}

	dev_printk(KERN_INFO, &pdev->dev, "claimed by uio_pci_proxy\n");
	return 0;

bad1:
	platform_set_drvdata(pdev, NULL);
	kfree(info);
bad:
	return ret;
}

static int remove(struct platform_device *pdev)
{
	struct uio_info *info = platform_get_drvdata(pdev);

	uio_unregister_device(info);

	platform_set_drvdata(pdev, NULL);

	kfree(info);

	dev_printk(KERN_INFO, &pdev->dev, "released by uio_pci_proxy\n");

	return 0;
}

static struct platform_driver pdriver = {
	.probe = probe,
	.remove = remove,
	.driver = {
		.name = "uio_multicast_pdriver",
		.owner = THIS_MODULE,
	},
};

static struct platform_device *pdevice;

/*
 * This 'jungle' is just because UIO present a convenient
 * interface to userspace. The module register itself to UIO by 
 * faking a platform device.
 */
int udrv_irq_multicast_init(void)
{
	int err,x;

	err = platform_driver_register(&pdriver);
	if(err)
		return err;

	pdevice = platform_device_alloc("uio_multicast_pdriver", 0);
	if (!pdevice){
		platform_driver_unregister(&pdriver); 	
		return -ENOMEM;
	}

	err = platform_device_add(pdevice);
	if(err){
		platform_device_put(pdevice);
		platform_driver_unregister(&pdriver);
		return err;
	}

	for(x=0;x<256;x++){
		atomic_set(&irq_mc[x].refcount,0);
		INIT_LIST_HEAD(&irq_mc[x].list);
	}

	UDRV_DPRINTK("");
	return 0;
}

void udrv_irq_multicast_cleanup(void)
{
	platform_device_unregister(pdevice);

	platform_driver_unregister(&pdriver);

	UDRV_DPRINTK("");
}


