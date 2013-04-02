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

// #include <linux/interrupt.h>//so we can sleep!
// #include <linux/wait.h>
// #include <linux/sched.h>

#include "modgpio.h"

#define SYSTIMER_MOD_AUTH "Techplex"
#define SYSTIMER_MOD_DESC "GPIO access control for Raspberry Pi"
#define SYSTIMER_MOD_SDEV "RPiGPIO" 		//supported devices
#define MOD_NAME "rpigpio" //name of the device file

static int st_open(struct inode *inode, struct file *filp);
static int st_release(struct inode *inode, struct file *filp);
static long st_ioctl(struct file *filp, unsigned int cmd, unsigned long arg);

//Global variables:
struct gpiomod_data {
	int mjr;
	struct class *cls;
	spinlock_t lock;

};

static struct gpiomod_data std = {
	.mjr = 0,
	.cls = NULL,
	.lock	= NULL,		//spinlock
};

//parameter
static int gpio_rpi_rev = 2; //version of the rpi

//Device special files have two numbers associated with them
// -major index into array
static const struct file_operations systimer_fops = {
	.owner			= THIS_MODULE,
	.open			= st_open,
	.release 		= st_release,
	.unlocked_ioctl = st_ioctl,
};

//! allow for parameters when inserting the module
module_param(gpio_rpi_rev, int, S_IRUSR|S_IWUSR|S_IRGRP);
MODULE_PARM_DESC(gpio_rpi_rev, "RPi Version");

//! handles user opening device special file
static int st_open(struct inode*inode, struct file *filp)
{
	return 0;	//todo: why don't we have to do anything here?
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
	uint32_t	val;
	struct stll *n = NULL;

	st = readl(__io_address(ST_BASE + 0x04));

	switch (cmd) {
		case SYSTIMER_DELAY:
			if (filp->f_flags&O_NONBLOCK) {
				return -EAGAIN;					//tell the user this call would block
			}
			get_user (cmp, (uint32_t __user *) arg);
			st_next = st + cmp;
			stll_insert (std.st_timell, st_next);

			n = stll_first (std.st_timell);

			spin_lock (&(std.lock));
			// Masking 64 to 32bit :: chop off the upper bits as the cmp register is only 32 bits
			writel ((n->next->t) & 0xFFFFFFFF,	__io_address (ST_BASE + 0x10));

			spin_unlock (&(std.lock));

			while (readl(__io_address(ST_BASE+0x04)) < st_next) { //(systimer value < next interrupt)

				prepare_to_wait(&(std.st_wq), &waiter, TASK_INTERRUPTIBLE);
				if (readl(__io_address(ST_BASE+0x04)) < st_next) {
					schedule();//go to sleep
				}
				finish_wait(&std.st_wq, &waiter);
				if (signal_pending(current)) {	//current is a pointer to current process (this)
					stll_delete (std.st_timell, st_next);
					return -ERESTARTSYS;//ctrl+c
				}
			}
			stll_delete (std.st_timell, st_next);
			return 0;
		case SYSTIMER_READ:
			put_user(st, (uint64_t __user *)arg);	//this is potentially problemsome, why?
			return 0;

		case SYSTIMER_LL_FIRST:
			n = stll_first(std.st_timell);
			put_user(n->t, (uint64_t __user *)arg);
			return 0;
		case SYSTIMER_ll_PRINT:
			stll_print(std.st_timell);	//this prints to the syslog
			return 0;
		case SYSTIMER_ll_INS:
			get_user(val, (uint32_t __user *)arg);
			stll_insert(std.st_timell, val);
			return 0;
		case SYSTIMER_ll_DEL:
			get_user(val, (uint32_t __user *)arg);
			stll_delete(std.st_timell, val);
			return 0;
	}
	//	return -EINVAL; //Error Message: invalid argument
	return -ENOTTY;	//Error Message: inappropriate IOCTL for device
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

1. register char device
2. Create class
3. Create device

*/
static int __init rpigpio_minit(void)
{
	struct device *dev;
	int ret;									//return value of request IRQ

	printk(KERN_INFO "Startup\n");
	//register char device
	std.mjr = register_chrdev(0, "systimer", &systimer_fops);//character device
	if (std.mjr < 0) {
		printk(KERN_ALERT "Cannot Register");
		return std.mjr;
	}
	printk(KERN_INFO "Major #%d\n", std.mjr);

	//Create class. What does a class do? - Organizes data
	std.cls = class_create(THIS_MODULE, "std.cls");
	if (IS_ERR(std.cls)) {
		printk(KERN_ALERT "Cannot get class\n");
		//Need to unregister
		unregister_chrdev(std.mjr, "systimer");	//Kerneldevelopes use gotos on errors
		return PTR_ERR(std.cls);					//Gets errrno code so one can lookup the error
	}

	//store a pointer to the st_devnode function
	//-its a callback when the device special file is actually being created
	std.cls->devnode = st_devnode;

	//Create Device													name of dev/spec.file
	dev = device_create(std.cls, NULL, MKDEV(std.mjr, 0), (void*)&std, "systimer");
	if (IS_ERR(dev)) {
		printk(KERN_ALERT "Cannot create device\n");
		//remove classs
		class_destroy(std.cls);
		//Unregister
		unregister_chrdev(std.mjr, "systimer");
		return PTR_ERR(dev);
	}

	printk(KERN_NOTICE "Systimer Loaded\n");
	printk(KERN_INFO "Mode Param: %d\n", gpio_rpi_rev); //passing parameters

	//init the spinlock
	spin_lock_init(&(std.lock));
	return 0;
}

static void __exit rpigpio_mcleanup(void)
{
	device_destroy(std.cls, MKDEV(std.mjr, 0));
	class_destroy(std.cls);
	unregister_chrdev(std.mjr, "systimer");

	printk(KERN_NOTICE "Goodbye\n");
}

module_init(rpigpio_minit);
module_exit(rpigpio_mcleanup);

MODULE_LICENSE("GPL");
MODULE_AUTHOR(SYSTIMER_MOD_AUTH);
MODULE_DESCRIPTION(SYSTIMER_MOD_DESC);
MODULE_SUPPORTED_DEVICE(SYSTIMER_MOD_SDEV);
