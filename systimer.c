
#include <linux/module.h>	//
#include <linux/kernel.h>	//printk
#include <linux/init.h>		//
#include <linux/stat.h>		//permissions
#include <linux/device.h>	//
#include <linux/fs.h>		//
#include <linux/err.h>		//Error checking macros

#define SYSTIMER_MOD_AUTH "Techplex"
#define SYSTIMER_MOD_DESC "SysTimer for Raspberry Pi"
#define SYSTIMER_MOD_SDEV "SysTimerRPi" //supported devices

static int sint __initdata = 0;
static int systimer_mode = 0;

static struct class *systimer_class = NULL;
static int systimer_major=0;
//Device special files have two numbers associated with them
// -major index into array
static const struct file_operations systimer_fops = {
	.owner=THIS_MODULE,
	.open=NULL,
	.release=NULL,
};

module_param(systimer_mode, int, S_IRUSR|S_IWUSR|S_IRGRP);
MODULE_PARM_DESC(systimer_mode, "Systimer mode");

static int __init rpisystimer_minit(void)
{
	struct device *dev;
	//register char device
	systimer_major = register_chrdev(0, "systimer", &systimer_fops);//character device
	if (systimer_major < 0) {
		printk(KERN_INFO "Cannot Register");
		return systimer_major;
	}
	printk(KERN_INFO "Major #%d\n", systimer_major);

	//Create class
	systimer_class = class_create(THIS_MODULE, "systimer_class");
	if (IS_ERR(systimer_class)) {
		printk(KERN_INFO "Cannot get class\n");
		//Need to unregister
		unregister_chrdev(systimer_major, "systimer");	//Kerneldevelopes use gotos on errors
		return PTR_ERR(systimer_class);		//Gets errro code so one can lookup the errpr
	}
	//Create Device															name of dev/spec.file
	dev = device_create(systimer_class, NULL, MKDEV(systimer_major, 0), NULL, "systimer");
	if (IS_ERR(dev)) {
		printk(KERN_INFO "Cannot create device\n");
		//remove classs
		class_destroy(systimer_class);
		//Unregister
		unregister_chrdev(systimer_major, "systimer");
		return PTR_ERR(dev);
	}	

	printk(KERN_NOTICE "Systimer Loaded\n");
	printk(KERN_INFO "Var: %d\n", sint);
	printk(KERN_INFO "Var2: %d\n", systimer_mode);
	return 0;
}

static void __exit rpisystimer_mcleanup(void)
{
	device_destroy(systimer_class, MKDEV(systimer_major, 0));
	class_destroy(systimer_class);
	unregister_chrdev(systimer_major, "systimer");

	printk(KERN_NOTICE "Goodbye\n");
}

module_init(rpisystimer_minit);
module_exit(rpisystimer_mcleanup);

MODULE_LICENSE("GPL");
MODULE_AUTHOR(SYSTIMER_MOD_AUTH);
MODULE_DESCRIPTION(SYSTIMER_MOD_DESC);
MODULE_SUPPORTED_DEVICE(SYSTIMER_MOD_SDEV);


