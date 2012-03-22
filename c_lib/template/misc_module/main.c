/*
 * This work is licensed under the terms of the GNU GPL, version 2.
 */

#include <linux/module.h>
#include <linux/sched.h>
#include <linux/errno.h>
#include <linux/miscdevice.h>
#include <linux/fcntl.h>

#include <linux/rtc.h>
#include <linux/init.h>
#include <linux/poll.h>
#include <linux/proc_fs.h>
#include <linux/mutex.h>
#include <linux/workqueue.h>

#define DRIVER_VERSION	"0.01"
#define DRIVER_AUTHOR	"Etienne Martineau <etmartin101@gmail.com>"
#define DRIVER_DESC	"template module"

#define DPRINTK(fmt, args...)	\
	do{	\
		if(debug) \
			printk(KERN_DEBUG "%s: " fmt, __func__ , ## args); \
} while (0)

static int debug=0;
MODULE_PARM_DESC(debug, "");
module_param(debug, bool, 0644);

static ssize_t read(struct file *file, char __user *buf,
			size_t count, loff_t *ppos)
{
	return -EINVAL;
}

static long ioctl(struct file *file,
			 unsigned int cmd, unsigned long arg)
{
	return -EINVAL;
}

static long unlocked_ioctl(struct file *file, unsigned int cmd,
				   unsigned long arg)
{
#if 0
	mutex_lock(&gen_rtc_mutex);
	ret = ioctl(file, cmd, arg);
	mutex_unlock(&gen_rtc_mutex);
#endif
	return -EINVAL;
}

static int open(struct inode *inode, struct file *file)
{
	return 0;
}

static int release(struct inode *inode, struct file *file)
{
	return 0;
}

static const struct file_operations fops = {
	.owner		= THIS_MODULE,
	.read		= read,
	.compat_ioctl	= ioctl,
	.unlocked_ioctl	= unlocked_ioctl,
	.open		= open,
	.release	= release,
};

static struct miscdevice dev =
{
	.minor		= MISC_DYNAMIC_MINOR,
	.name		= "template",
	.fops		= &fops,
};

static int __init init(void)
{
	int retval;

	retval = misc_register(&dev);
	if (retval < 0)
		return retval;

	pr_info(DRIVER_DESC " version: " DRIVER_VERSION);

	return 0;
}

static void __exit cleanup(void)
{
	misc_deregister(&dev);
}

module_init(init);
module_exit(cleanup);

MODULE_VERSION(DRIVER_VERSION);
MODULE_LICENSE("GPL v2");
MODULE_AUTHOR(DRIVER_AUTHOR);
MODULE_DESCRIPTION(DRIVER_DESC);

