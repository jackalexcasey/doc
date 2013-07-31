/* spawns a kernel thread to send NMIs to a specific vcpu of a
 * given VM every 4 msec
 *
 * Walks the files list of the VM looking for a kvm-vcpu name.
 * vcpu data is located in the private_data of the corresponding
 * file struct.
 *
 * WARNING: no reference is taken to the vcpu data structure so
 *          if the VM dies it will be referencing freed memory.
 *
 * start: insmod kvmnmi.ko vmpid=9803 vcpu_id=0
 * stop:  rmmod kvmnmi
 *
 * Written/tested against the 2.6.35.14 kernel
 *
 * David Ahern
 * daahern@cisco.com
 * July 25, 2013
 */

#include <linux/version.h>
#include <linux/module.h>
#include <linux/types.h>
#include <linux/kernel.h>
#include <linux/kdebug.h>
#include <linux/smp.h>
#include <linux/sched.h>
#include <linux/kvm_host.h>
#include <linux/fs.h>
#include <linux/fdtable.h>

/* time between NMIs in msec - converted to nanosec */
#define TOUT_NSEC  10 * NSEC_PER_MSEC 

pid_t vmpid;
module_param(vmpid, int, 0444);
int vcpu_id = -1;
module_param(vcpu_id, int, 0444);

static pid_t nmi_thread;
static struct task_struct *vm_task;

static struct task_struct *my_find_task(pid_t pid)
{
	// return find_task_by_vpid(pid);
	struct pid *p;
	struct task_struct *t = NULL;

	p = find_get_pid(pid);
	if (p)
		t = pid_task(p, PIDTYPE_PID);
		
	return t;
}


static int vcpu_pdic_nmi(void *arg)
{
	ktime_t expires;
	unsigned long count;
	char comm[32];

	snprintf(comm, sizeof(comm), "kvm-nmi" );
	daemonize(comm);
	allow_signal(SIGTERM);

	(void) get_fs();
	set_fs(get_ds());

	count = 1*NSEC_PER_SEC/TOUT_NSEC;
	while (1) {
		/* inject an NMI every 10 msec */
		expires = ktime_set(0, TOUT_NSEC);
		set_current_state(TASK_INTERRUPTIBLE);
		schedule_hrtimeout(&expires, HRTIMER_MODE_REL);

		if (signal_pending(current))
			break;
	}

	nmi_thread = 0;
	printk("%s exiting\n", comm);

	return 0;
}

static int __init kvmnmi_init(void)
{
	nmi_thread = kernel_thread(vcpu_pdic_nmi, NULL, CLONE_KERNEL);
	if (nmi_thread <= 0) {
		printk("Failed to start kernel thread\n");
		return -1;
	}

	return 0;
}

static void __exit kvmnmi_fini(void)
{
	struct task_struct *t;

	while (nmi_thread > 0) {

		t = my_find_task(nmi_thread);
		if (t == NULL)
			break;

		send_sig(SIGTERM, t, 0);

		set_current_state(TASK_INTERRUPTIBLE);
		schedule_timeout(4*HZ);
		set_current_state(TASK_RUNNING);
	}
}

module_init(kvmnmi_init);
module_exit(kvmnmi_fini);

MODULE_LICENSE("GPL");
