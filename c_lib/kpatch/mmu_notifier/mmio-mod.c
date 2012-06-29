#if 0
	req = kzalloc(GFP_KERNEL, sizeof(struct user_mmio_req));
	if(!req)
		return;

	req->offset = offset;
	req->size = size;
	req->addr = addr;
	req->mm = mm;

	spin_lock(&user_mmio_lock);
	list_add(&req->list, &user_mmio_list);
	spin_unlock(&user_mmio_lock);

	if(!work_pending(&user_mmio_work))
		schedule_work(&user_mmio_work);
#endif

#if 0
#include <linux/mmu_notifier.h>
#include <linux/sched.h>

/*
 * User mmio tracer
 */
struct user_mmio_req{
	struct list_head list;
	resource_size_t offset;
	unsigned long size;
	void __iomem *addr;
	struct mm_struct *mm;
};

static void do_user_mmio_work(struct work_struct *work);
static void notifier_release(struct mmu_notifier *mn,
				     struct mm_struct *mm);

static DEFINE_SPINLOCK(user_mmio_lock);
static LIST_HEAD(user_mmio_list);
static DECLARE_WORK(user_mmio_work, do_user_mmio_work);

static const struct mmu_notifier_ops mmu_notifier_ops = {
	.release = notifier_release,
};
static struct mmu_notifier mn = {
	.ops = &mmu_notifier_ops,
};

/*
 * This is a notification that mm is being unmapped
 * at this stage the PTEs are still valid so we can get
 * a reference to it; clean the PTE_PRESENT setting and 
 * let the normal execution continue.
 */
static void notifier_release(struct mmu_notifier *mn,
				     struct mm_struct *mm)
{
	printk("HHHHHHHHHEEEEEEEEEEEEEEEE!!!!\n");
	mmu_notifier_unregister(mn,mm);
	printk("111HHHHHHHHHEEEEEEEEEEEEEEEE!!!!\n");
}

static void do_user_mmio_work(struct work_struct *work)
{
	struct user_mmio_req *req, *tmp, *found;

again:
	found = NULL;

	/* We use the list as a fifo really */
	spin_lock(&user_mmio_lock);
	list_for_each_entry_safe(req, tmp, &user_mmio_list, list) {
		list_del(&req->list);
		found = req;
		break;
	}
	spin_unlock(&user_mmio_lock);

	if (found) {
		pr_debug(NAME "user_mmiotrace_ioremap_*(0x%llx, 0x%lx) = %p / %p\n",
				(unsigned long long)found->offset, found->size, found->addr, found->mm);
		/*
		 * BUG
		 * With remap PTE range the same mm can be used ( when large chunk...
		 * so it cause a lock on the mmu_notifier_register()
		 *
		 * Cannot register more than one
		 * Yes you can there is nothing that blocks it
		 */
		if(mmu_notifier_register(&mn, found->mm)<0){
			printk("ERROR MMU notifier");
		}
		kfree(found);
		goto again;
	}
}
#endif


