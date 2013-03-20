#include <linux/module.h>	//module stuff
#include <linux/kernel.h>	//printk
#include <linux/init.h>		//__init
#include <linux/stat.h>		//permissions
#include <linux/device.h>	//Device class stuff
#include <linux/fs.h>		//File operation structures
#include <linux/err.h>		//Error checking macros

#include <linux/ioctl.h>
#include <linux/io.h>		//read and write from the memory
#include <asm/uaccess.h>	//translation from userspace ptr to kernelspace
#include <mach/platform.h>	//pull address of system timer

#include <linux/interrupt.h>//so we can sleep!
#include <linux/wait.h>
#include <linux/sched.h>

#include "systimer.h"
#include "systimerll.h"

#define SYSTIMER_MOD_AUTH "Techplex"
#define SYSTIMER_MOD_DESC "SysTimer for Raspberry Pi"
#define SYSTIMER_MOD_SDEV "SysTimerRPi" 		//supported devices

static int st_open(struct inode *inode, struct file *filp);
static int st_release(struct inode *inode, struct file *filp);
static long st_ioctl(struct file *filp, unsigned int cmd, unsigned long arg);

//Global variables:
struct systimer_data {
	int st_mjr;
	struct class *st_cls;
	int st_irq;
	wait_queue_head_t st_wq;
	struct stll_data *st_lld;
};

static struct systimer_data std = {
	.st_mjr = 0,
	.st_cls = NULL,
	.st_irq = 0,								//successfully allocated interrupt
	.st_lld = &stlld,							//get a reference to the ll data for the interrupt
};

static int systimer_mode = 0;

//Device special files have two numbers associated with them
// -major index into array
static const struct file_operations systimer_fops = {
	.owner			= THIS_MODULE,
	.open			= st_open,
	.release 		= st_release,
	.unlocked_ioctl = st_ioctl,
};

//! allow for parameters when inserting the module
module_param(systimer_mode, int, S_IRUSR|S_IWUSR|S_IRGRP); //@q: not sure what these guys are?
MODULE_PARM_DESC(systimer_mode, "Systimer mode");

//! handles user opening device special file
static int st_open(struct inode*inode, struct file *filp)
{
	return 0;
}

//! handles user closing the device special file
static int st_release(struct inode *inode, struct file *filp)
{
	return 0;
}

//! Processes IOCTL calls
static long st_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
	uint64_t st, st_next;
	uint32_t cmp;
	DEFINE_WAIT(waiter); //add ourselves to wait queue
	struct stll *temp;
	uint32_t	val;

	st = readl(__io_address(ST_BASE + 0x04));

	switch (cmd) {
		case SYSTIMER_DELAY:
			if (filp->f_flags&O_NONBLOCK) {
				return -EAGAIN;					//tell the user this call would block
			}
			get_user(cmp, (uint32_t __user *)arg);
			st_next = st + cmp;

			//need protection here:
			//-Race conditions with mutliple programs
			//-chop off the upper bits as the cmp register is only 32 bits
			writel(st_next&0xFFFFFFFF, __io_address(ST_BASE + 0x10));

			while (readl(__io_address(ST_BASE+0x04)) < st_next) { //(systimer value < next interrupt)

				prepare_to_wait(&(std.st_wq), &waiter, TASK_INTERRUPTIBLE);
				if (readl(__io_address(ST_BASE+0x04)) < st_next) {
					schedule();//go to sleep
				}
				finish_wait(&std.st_wq, &waiter);
				if (signal_pending(current)) {	//current is a pointer to current process (this)
					return -ERESTARTSYS;//ctrl+c
				}
			}
			return 0;
		case SYSTIMER_READ:
			put_user(st, (uint64_t __user *)arg);	//this is potentially problemsome, why?
			return 0;

		case SYSTIMER_LL_FIRST:
			temp = stll_first(std.st_lld->st_timell);
			put_user(temp->t, (uint64_t __user *)arg);
			return 0;
		case SYSTIMER_ll_PRINT:
			stll_print(std.st_lld->st_timell);	//this prints to the syslog
			return 0;
		case SYSTIMER_ll_INS:
			get_user(val, (uint32_t __user *)arg);
			stll_insert(std.st_lld->st_timell, val);
			return 0;
		case SYSTIMER_ll_DEL:
			get_user(val, (uint32_t __user *)arg);
			stll_delete(std.st_lld->st_timell, val);
			return 0;
	}
	//	return -EINVAL; //Error Message: invalid argument
	return -ENOTTY;	//Error Message: inappropriate IOCTL for device
}
//! ISR Interrupt Handler
static irqreturn_t st_irqhandler(int irq, void *dev_id)
{
	struct systimer_data * st_dev = (struct systimer_data *)dev_id;
	struct stll *n;

	disable_irq(IRQ_TIMER3);					//don't disable interrupts for too long
	writel(0x02, __io_address(ST_BASE + 0x00));	//what does this disable
	enable_irq(IRQ_TIMER3);

	n = stll_first (st_dev->st_lld->st_timell);
	spin_lock (&(st_dev->st_lld->st_lock));

	if (n->next->next && n->next != NULL) {
		writel ((n->next->next->t) & 0xFFFFFFFF, __io_address (ST_BASE + 0x10));
	}

	spin_unlock (&(st_dev->st_lld->st_lock));

	wake_up_interruptible(&(st_dev->st_wq));	//wakes up all tasks on queue. asks to be scheduled again. When time is allocated, code is entered from "schedule()" call

	return IRQ_HANDLED;
}

//! Sets permissions correctly on created device special file
static char *st_devnode(struct device *dev, umode_t *mode)
{
	if (mode) *mode = 0666;//adding a leading 0 makes number octal
	return NULL;
}
//! initialize the driver.
/*!
Couple of things to do here:
1. Init the wait queue
2. register char device
3. Create class
4. Create device
5. Request IRQ (Interrupt) for our delays
*/
static int __init rpisystimer_minit(void)
{
	struct device *dev;
	int ret;									//return value of request IRQ
	struct stll *temp;

	init_waitqueue_head(&std.st_wq);			//create the wait queue

	printk(KERN_INFO "Startup\n");
	//register char device
	std.st_mjr = register_chrdev(0, "systimer", &systimer_fops);//character device
	if (std.st_mjr < 0) {
		printk(KERN_ALERT "Cannot Register");
		return std.st_mjr;
	}
	printk(KERN_INFO "Major #%d\n", std.st_mjr);

	//Create class. What does a class do? - Organizes data
	std.st_cls = class_create(THIS_MODULE, "std.st_cls");
	if (IS_ERR(std.st_cls)) {
		printk(KERN_ALERT "Cannot get class\n");
		//Need to unregister
		unregister_chrdev(std.st_mjr, "systimer");	//Kerneldevelopes use gotos on errors
		return PTR_ERR(std.st_cls);					//Gets errrno code so one can lookup the error
	}

	//store a pointer to the st_devnode function
	//-its a callback when the device special file is actually being created
	std.st_cls->devnode = st_devnode;

	//Create Device													name of dev/spec.file
	dev = device_create(std.st_cls, NULL, MKDEV(std.st_mjr, 0), (void*)&std, "systimer");
	if (IS_ERR(dev)) {
		printk(KERN_ALERT "Cannot create device\n");
		//remove classs
		class_destroy(std.st_cls);
		//Unregister
		unregister_chrdev(std.st_mjr, "systimer");
		return PTR_ERR(dev);
	}

	printk(KERN_NOTICE "Systimer Loaded\n");
	printk(KERN_INFO "Mode Param: %d\n", systimer_mode); //passing parameters

	ret = request_irq(IRQ_TIMER1, st_irqhandler, 0, "st_timer1", (void*)&std);
	if (ret) {
		printk(KERN_ALERT "Cannot get IRQ\n");
	} else {
		printk(KERN_INFO "IRQ Request Granted\n");
		std.st_irq = IRQ_TIMER1;
	}
	//init the spinlock
	spin_lock_init(&(std.st_lld->st_lock));
	// init the linked list
	if (std.st_lld == NULL) {
		printk(KERN_NOTICE "std.st_lld ITS NULL\n");
		return 0;
	}
	if (std.st_lld->st_timell == NULL)
		printk(KERN_NOTICE "std.st_lld->st_timell ITS NULL\n");


	std.st_lld->st_timell = stll_init();
	return 0;
}

static void __exit rpisystimer_mcleanup(void)
{
	if (std.st_irq) { 							//if we did get interrupt registed
		free_irq(IRQ_TIMER1, (void *)&std);		//free it
	}

	device_destroy(std.st_cls, MKDEV(std.st_mjr, 0));
	class_destroy(std.st_cls);
	unregister_chrdev(std.st_mjr, "systimer");

	printk(KERN_NOTICE "Goodbye\n");
}

module_init(rpisystimer_minit);
module_exit(rpisystimer_mcleanup);

MODULE_LICENSE("GPL");
MODULE_AUTHOR(SYSTIMER_MOD_AUTH);
MODULE_DESCRIPTION(SYSTIMER_MOD_DESC);
MODULE_SUPPORTED_DEVICE(SYSTIMER_MOD_SDEV);
