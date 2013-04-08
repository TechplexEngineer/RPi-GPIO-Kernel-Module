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

#include <linux/types.h>

// #include <linux/interrupt.h>//so we can sleep!
// #include <linux/wait.h>
#include <linux/sched.h>		//for current->pid

#include "modgpio.h"

#define RPIGPIO_MOD_AUTH "Techplex"
#define RPIGPIO_MOD_DESC "GPIO access control for Raspberry Pi"
#define RPIGPIO_MOD_SDEV "RPiGPIO" 		//supported devices
#define MOD_NAME "rpigpio" //name of the device file

static int st_open(struct inode *inode, struct file *filp);
static int st_release(struct inode *inode, struct file *filp);
static long st_ioctl(struct file *filp, unsigned int cmd, unsigned long arg);


#define PIN_NULL_PID 5	//signifies not a valid pin. Unsigned number
#define PIN_UNASSN 0	//signifies pin available
#define PIN_ARRAY_LEN 32

//Global variables:
struct gpiomod_data {
	int mjr;
	struct class *cls;
	spinlock_t lock;
	uint32_t pins[PIN_ARRAY_LEN];
};

static struct gpiomod_data std = {
	.mjr = 0,
	.cls = NULL,
	//.lock= NULL
	.pins = {			//note this is for rpiv2
		PIN_NULL_PID,
		PIN_NULL_PID,
		PIN_UNASSN,	//pin 2
		PIN_UNASSN,	//pin 3
		PIN_UNASSN,	//pin 4
		PIN_NULL_PID,
		PIN_NULL_PID,
		PIN_UNASSN,	//pin 7
		PIN_UNASSN,	//pin 8
		PIN_UNASSN,	//pin 9
		PIN_UNASSN,	//pin 10
		PIN_UNASSN,	//pin 11
		PIN_NULL_PID,
		PIN_NULL_PID,
		PIN_UNASSN,	//pin 14
		PIN_UNASSN,	//pin 15
		PIN_NULL_PID,
		PIN_UNASSN,	//pin 17
		PIN_UNASSN,	//pin 18
		PIN_NULL_PID,
		PIN_NULL_PID,
		PIN_NULL_PID,
		PIN_UNASSN,	//pin 22
		PIN_UNASSN,	//pin 23
		PIN_UNASSN,	//pin 24
		PIN_UNASSN,	//pin 25
		PIN_NULL_PID,
		PIN_UNASSN,	//pin 27
		PIN_UNASSN,	//pin 28
		PIN_UNASSN,	//pin 29
		PIN_UNASSN,	//pin 30
		PIN_UNASSN,	//pin 31
	}
};

//parameter
static int gpio_rpi_rev = 2; //version of the rpi

//Device special files have two numbers associated with them
// -major index into array
static const struct file_operations gpio_fops = {
	.owner			= THIS_MODULE,
	.open			= st_open,
	.release 		= st_release,
	.unlocked_ioctl = st_ioctl,
};

//! allow for parameters when inserting the module
module_param(gpio_rpi_rev, int, S_IRUSR|S_IWUSR|S_IRGRP);
MODULE_PARM_DESC(gpio_rpi_rev, "RPi Version");

//! handles user opening device special file
//Open and release are called each time 
static int st_open(struct inode*inode, struct file *filp)
{
	return 0;	//todo: why don't we have to do anything here?
}

//! handles user closing the device special file
static int st_release(struct inode *inode, struct file *filp)
{
	int i;
	//check to see if the releasing process has any pins checked out, and free them
	for (i=0; i<PIN_ARRAY_LEN; i++) {
		if (std.pins[i] == current->pid) {
			printk(KERN_DEBUG "[FREE] Pin:%d From:%d\n", i, current->pid);
			std.pins[i] = PIN_UNASSN;
		}
	}
	return 0;
}

#define GPFSEL0 0x00
#define GPFSEL1 0x04
#define GPFSEL2 0x08
#define GPFSEL3 0x0C
#define GPFSEL4 0x10
#define GPFSEL5 0x14

#define GPSET0 0x1C
#define GPSET1 0x20

#define GPCLR0 0x28
#define GPCLR1 0x2C

#define GPLEV0 0x34
#define GPLEV1 0x38

//! Processes IOCTL calls
static long st_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
	int pin;	//used in read, request, free
	unsigned long ret;	//return value for copy to/from user
	uint32_t val;
	uint8_t flag;
	//uint32_t currVal;
	struct gpio_data_write wdata;	//write data
	struct gpio_data_mode  mdata;	//mode data
	switch (cmd) {
		case GPIO_READ:
			//@todo is there a reason we didn't check the return value of get_user?
			get_user (pin, (int __user *) arg);

			val = readl(__io_address (GPIO_BASE + GPLEV0 + (pin/32)*4)); //move to next long if pin/32 ==1
			flag = val >> (pin%32); 	//right shift by the remainder
			flag &= 0x01;				//clear upper bits
			printk(KERN_INFO "[READ] Pin: %d Val:%d\n", pin, flag);
			//printk(KERN_DEBUG "Test: 0x%.8lX Temp: 0x%x\n",val, flag);
			put_user(flag, (uint8_t __user *)arg);

			return 0;
		case GPIO_WRITE:
			//Check permissions
			ret = copy_from_user(&wdata, (struct gpio_data_write __user *)arg, sizeof(struct gpio_data_write));
			if (ret != 0) {
				printk(KERN_DEBUG "[WRITE] Error copying data from userspace\n");
				return -EFAULT;
			}
			if (std.pins[wdata.pin] != current->pid) { //make sure the process has it checked out
				return -EACCES;	//Permission denied
			}
			printk(KERN_INFO "[WRITE] Pin: %d Val:%d\n", wdata.pin, wdata.data);
			printk(KERN_DEBUG "info: %lX\n", __io_address (GPIO_BASE + GPSET0));

			if (wdata.data == 1)//set
				writel (1<<wdata.pin,	__io_address (GPIO_BASE + GPSET0));
			else //clear
				writel (1<<wdata.pin,	__io_address (GPIO_BASE + GPCLR0));

			//printk(KERN_INFO "val2: 0x%lX addy2: 0x%lX\n", currVal, __io_address (GPIO_BASE + 0x1C));
			return 0;
		case GPIO_REQUEST:
			//@todo Need some locking here
			get_user (pin, (int __user *) arg);
			if (pin > PIN_ARRAY_LEN || pin < 0 || std.pins[pin] == PIN_NULL_PID) {
				return -EFAULT;	//Bad address
			} else if (std.pins[pin] == PIN_UNASSN) {
				std.pins[pin] = current->pid;
				printk(KERN_DEBUG "[REQUEST] Pin:%d Assn To:%d\n", pin, current->pid);
				return 0;
			}
			return -EBUSY;	//Device or resource busy

		case GPIO_FREE:
			get_user (pin, (int __user *) arg);
			if (pin > PIN_ARRAY_LEN || pin < 0 || std.pins[pin] == PIN_NULL_PID) {
				return -EFAULT;	//Bad address
			} else if (std.pins[pin] == current->pid) {
				std.pins[pin] = PIN_UNASSN;
				printk(KERN_DEBUG "[FREE] Pin:%d From:%d\n", pin, current->pid);
				return 0;
			}
			return -EFAULT;	//Bad address??
		case GPIO_TOGGLE:
			get_user (pin, (int __user *) arg);
			///check for write perms, else die
			if (pin > PIN_ARRAY_LEN || pin < 0 || std.pins[pin] == PIN_NULL_PID) {
				return -EFAULT;	//Bad address
			} else if (std.pins[pin] == current->pid) {
				val = readl(__io_address (GPIO_BASE + GPLEV0 + (pin/32)*4)); //move to next long if pin/32 ==1
				flag = val >> (pin%32); 	//right shift by the remainder
				flag &= 0x01;				//clear upper bits
				printk(KERN_DEBUG "[TOGGLE] Pin:%d From:%.1d To:%.1d\n", pin, flag, flag?0:1);
				if (flag != 1)//set
					writel (1<<pin,	__io_address(GPIO_BASE + GPSET0));
				else //clear
					writel (1<<pin,	__io_address(GPIO_BASE + GPCLR0));
				put_user(flag?0:1, (uint8_t __user *)arg);
				return 0;
			}
			return -EACCES;	//Permission denied
		case GPIO_MODE:

			ret = copy_from_user(&mdata, (struct gpio_data_mode __user *)arg, sizeof(struct gpio_data_mode));
			if (ret != 0) {
				printk(KERN_DEBUG "[MODE] Error copying data from userspace\n");
				return -EFAULT;
			}
			//validate pin
			if (mdata.pin > 31 || mdata.pin < 0 || std.pins[mdata.pin] == PIN_NULL_PID) {
				return -EFAULT;	//Bad address
			} else if (std.pins[mdata.pin] == current->pid) { //make sure we have access
				if(mdata.data == MODE_INPUT) {
					writel(~(7<<((mdata.pin %10)*3)) & readl(__io_address(GPIO_BASE + (0x04)*(mdata.pin /10))), __io_address(GPIO_BASE + (0x04)*(mdata.pin/10))); // Set as input 
					printk(KERN_DEBUG "GPIO - Pin %d set as Input\n", mdata.pin);
				} else if (mdata.data == MODE_OUTPUT) {
					//printk(KERN_DEBUG "GPIO - Writing %lx to %lx - val: %lx\n", (val & ~(7<<(((gtmp->pin)%10)*3))), __io_address(GPIO_BASE + (0x04)*(pin/10)), val);
					writel(~(7<<((mdata.pin %10)*3)) & readl(__io_address(GPIO_BASE + (0x04)*(mdata.pin /10))), __io_address(GPIO_BASE + (0x04)*(mdata.pin/10))); // Set as input to bit clear
					writel(1<<((mdata.pin % 10)*3) | readl(__io_address(GPIO_BASE + (0x04)*(mdata.pin/10))), __io_address(GPIO_BASE + (0x04)*(mdata.pin/10)));    // Set pin as output
					printk(KERN_DEBUG "GPIO - Pin %d set as Output\n", mdata.pin);
				}
				return 0;
			}
			return -EACCES;	//Permission denied

		default:
			return -ENOTTY;	//Error Message: inappropriate IOCTL for device
	}
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

	printk(KERN_INFO "[gpio] Startup\n");
	//register char device
	std.mjr = register_chrdev(0, MOD_NAME, &gpio_fops);//character device
	if (std.mjr < 0) {
		printk(KERN_ALERT "[gpio] Cannot Register");
		return std.mjr;
	}
	printk(KERN_INFO "[gpio] Major #%d\n", std.mjr);

	//Create class. What does a class do? - Organizes data
	std.cls = class_create(THIS_MODULE, "std.cls");
	if (IS_ERR(std.cls)) {
		printk(KERN_ALERT "[gpio] Cannot get class\n");
		//Need to unregister
		unregister_chrdev(std.mjr, MOD_NAME);	//Kerneldevelopes use gotos on errors
		return PTR_ERR(std.cls);					//Gets errrno code so one can lookup the error
	}

	//store a pointer to the st_devnode function
	//-its a callback when the device special file is actually being created
	std.cls->devnode = st_devnode;

	//Create Device													name of dev/spec.file
	dev = device_create(std.cls, NULL, MKDEV(std.mjr, 0), (void*)&std, MOD_NAME);
	if (IS_ERR(dev)) {
		printk(KERN_ALERT "[gpio] Cannot create device\n");
		//remove classs
		class_destroy(std.cls);
		//Unregister
		unregister_chrdev(std.mjr, MOD_NAME);
		return PTR_ERR(dev);
	}

	printk(KERN_INFO "[gpio] RPi Version: %d\n", gpio_rpi_rev); //passing parameters
	printk(KERN_INFO "[gpio] %s Loaded\n", MOD_NAME);

	//init the spinlock
	spin_lock_init(&(std.lock));
	return 0;
}

static void __exit rpigpio_mcleanup(void)
{
	device_destroy(std.cls, MKDEV(std.mjr, 0));
	class_destroy(std.cls);
	unregister_chrdev(std.mjr, MOD_NAME);

	printk(KERN_NOTICE "[gpio] Goodbye\n");
}

module_init(rpigpio_minit);
module_exit(rpigpio_mcleanup);

MODULE_LICENSE("GPL");
MODULE_AUTHOR(RPIGPIO_MOD_AUTH);
MODULE_DESCRIPTION(RPIGPIO_MOD_DESC);
MODULE_SUPPORTED_DEVICE(RPIGPIO_MOD_SDEV);
