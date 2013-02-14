
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/stat.h>

#define SYSTIMER_MOD_AUTH "Techplex"
#define SYSTIMER_MOD_DESC "SysTimer for Raspberry Pi"
#define SYSTIMER_MOD_SDEV "SysTimerRPi" //supported devices

static int sint __initdata = 0;
static int systimer_mode = 0;

module_param(systimer_mode, int, S_IRUSR|S_IWUSR|S_IRGRP);
MODULE_PARM_DESC(systimer_mode, "Systimer mode");

static int __init rpisystimer_minit(void)
{
	printk(KERN_NOTICE "Systimer Loaded\n");
	printk(KERN_INFO "Var: %d\n", sint);
	printk(KERN_INFO "Var2: %d\n", systimer_mode);
	return 0;
}

static void __exit rpisystimer_mcleanup(void)
{
	printk(KERN_NOTICE "Goodbye\n");
	return;
}

module_init(rpisystimer_minit);
module_exit(rpisystimer_mcleanup);

MODULE_LICENSE("GPL");
MODULE_AUTHOR(SYSTIMER_MOD_AUTH);
MODULE_DESCRIPTION(SYSTIMER_MOD_DESC);
MODULE_SUPPORTED_DEVICE(SYSTIMER_MOD_SDEV);


