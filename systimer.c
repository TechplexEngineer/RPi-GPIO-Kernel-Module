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

#include <linux/interrupt.h>
#include <linux/wait.h>
#include <linux/sched.h>

#include "systimer.h"

#define SYSTIMER_MOD_AUTH "Techplex"
#define SYSTIMER_MOD_DESC "SysTimer for Raspberry Pi"
#define SYSTIMER_MOD_SDEV "SysTimerRPi" //supported devices

static int st_open(struct inode *inode, struct file *filp);
static int st_release(struct inode *inode, struct file *filp);
static long st_ioctl(struct file *filp, unsigned int cmd, unsigned long arg);

struct systimer_data {
	int st_mjr;
	struct class *st_cls;
	int st_irq;
	wait_queue_head_t st_wq;
};

static struct systimer_data std = {
	.st_mjr = 0,
	.st_cls = NULL,
	.st_irq=0,					//successfully allocated interrupt
};

// static int sint __initdata = 0;
static int systimer_mode = 0;

///static struct class *std.st_cls = NULL;
///static int std.st_mjr=0;

//Device special files have two numbers associated with them
// -major index into array
static const struct file_operations systimer_fops = {
	.owner			= THIS_MODULE,
	.open			= st_open,
	.release 		= st_release,
	.unlocked_ioctl = st_ioctl,
};

module_param(systimer_mode, int, S_IRUSR|S_IWUSR|S_IRGRP);
MODULE_PARM_DESC(systimer_mode, "Systimer mode");

//! handles user opening device special file
static int st_open(struct inode*inode, struct file *filp)
{
	//How to interact with your device
//	if ((filp->f_flags & O_ACCMODE) == O_WRONLY)
//		return -EOPNOTSUPP; //Operation not supported
//	if ((filp->f_flags & O_ACCMODE) == O_RDWR)
//		return -EOPNOTSUPP; //Operation not supported
	return 0; //all good
}

//! handles user closing the device special file
static int st_release(struct inode *inode, struct file *filp)
{
	return 0;
}


static long st_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
	uint64_t st, st_next;
	uint32_t cmp;
	DEFINE_WAIT(waiter); //add ourselves to wait queue

	printk(KERN_INFO "IOCTL\n");

	st = readl(__io_address(ST_BASE + 0x04));

	switch (cmd) {
		case SYSTIMER_DELAY:
			printk(KERN_INFO "DELAY\n");
			if (filp->f_flags&O_NONBLOCK) {
				return -EAGAIN;		//This call would block
			}
			get_user(cmp, (uint32_t __user *)arg);
			//printk(KERN_INFO "Arg: %lu\n", cmp);
			st_next = st + cmp;

			//writel(st_next&0xFFFFFFFF, __io_address(ST_BASE + 0x10));
			writel(st_next, __io_address(ST_BASE + 0x10));	/* stc3 */

			//make sure our wait is still valid
			// if (readl(__io_address(ST_BASE+0x04)) < st_next) {
			// 	prepare_to_wait(&(std.st_wq), &waiter, TASK_INTERRUPTIBLE);
			// 	schedule();//go to sleep
			// 	finish_wait(&std.st_wq, &waiter);
			// } else {
			// 	//wasn't able to wait
			// 	return 0;
			// }



			while (readl(__io_address(ST_BASE+0x04)) < st_next) { //(systimer value < next interrupt)
				printk(KERN_INFO "wq: %p\tw: %p\n", &(std.st_wq), &waiter);
				prepare_to_wait(&(std.st_wq), &waiter, TASK_INTERRUPTIBLE);
				if (readl(__io_address(ST_BASE+0x04)) < st_next) {
					printk(KERN_ALERT "Going to sleep\n\n");
					schedule();//go to sleep
					printk(KERN_ALERT "Waking Up\n\n");
				}
				finish_wait(&std.st_wq, &waiter);
				if (signal_pending(current)) { //current is a pointer to current process (this)
					return -ERESTARTSYS;
				}
			}
			return 0;
		case SYSTIMER_READ:
			//printk(KERN_INFO "READ\n");
			put_user(st, (uint64_t __user *)arg);	//this is potentially problemsome
			return 0;
	}
	//	return -EINVAL; //invalid arg
	return -ENOTTY;	//Inappropriate IOctl for device
}
//! ISR
static irqreturn_t st_irqhandler(int irq, void *dev_id)
{
	struct systimer_data * st_dev = (struct systimer_data *)dev_id;
	printk(KERN_ALERT "Interrupt\n\n");

	disable_irq(IRQ_TIMER3);		//don't disable interrupts for too long
	writel(0x02, __io_address(ST_BASE + 0x00));
	//writel(1 << 3, __io_address(ST_BASE + 0x00));	/* stcs clear timer int */
	enable_irq(IRQ_TIMER3);

	wake_up_interruptible(&(st_dev->st_wq));

	return IRQ_HANDLED;
}

//! /brief Sets permissions correctly on created device special file
static char *st_devnode(struct device *dev, umode_t *mode)
{
	if (mode) *mode = 0666;//adding a 0 makes number octal
	return NULL;
}

static int __init rpisystimer_minit(void)
{
	struct device *dev;
	int ret;				//return value of request IRQ
	
	printk(KERN_ALERT "Startup\n\n");
	//register char device
	std.st_mjr = register_chrdev(0, "systimer", &systimer_fops);//character device
	if (std.st_mjr < 0) {
		printk(KERN_INFO "Cannot Register");
		return std.st_mjr;
	}
	printk(KERN_INFO "Major #%d\n", std.st_mjr);

	//Create class
	std.st_cls = class_create(THIS_MODULE, "std.st_cls");
	if (IS_ERR(std.st_cls)) {
		printk(KERN_INFO "Cannot get class\n");
		//Need to unregister
		unregister_chrdev(std.st_mjr, "systimer");	//Kerneldevelopes use gotos on errors
		return PTR_ERR(std.st_cls);		//Gets errro code so one can lookup the errpr
	}

	std.st_cls->devnode = st_devnode;

	//Create Device													name of dev/spec.file
	dev = device_create(std.st_cls, NULL, MKDEV(std.st_mjr, 0), (void*)&std, "systimer");
	if (IS_ERR(dev)) {
		printk(KERN_INFO "Cannot create device\n");
		//remove classs
		class_destroy(std.st_cls);
		//Unregister
		unregister_chrdev(std.st_mjr, "systimer");
		return PTR_ERR(dev);
	}

	printk(KERN_NOTICE "Systimer Loaded\n");
	// printk(KERN_INFO "Var: %d\n", sint);
	printk(KERN_INFO "Mode Param: %d\n", systimer_mode); //passing parameters


	ret = request_irq(IRQ_TIMER1, st_irqhandler, 0, "st_timer1", (void*)&std);
	if (ret) {
		printk(KERN_INFO "Cannot get IRQ\n");
	} else {
		printk(KERN_INFO "IRQ Request Granted\n");
		std.st_irq = IRQ_TIMER1;
	}

	init_waitqueue_head(&std.st_wq);

	return 0;
}

static void __exit rpisystimer_mcleanup(void)
{
	if (std.st_irq) { //if we did get interrupt registed
		free_irq(IRQ_TIMER1, (void *)&std);
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


