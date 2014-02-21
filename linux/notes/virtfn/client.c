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

