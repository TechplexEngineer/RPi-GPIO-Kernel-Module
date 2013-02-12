
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>


static int __init rpisystimer_minit(void)
{
	printk(KERN_NOTICE "Systimer Loaded\n");
	return 0;
}

static void __exit rpisystimer_mcleanup(void)
{
	printk(KERN_NOTICE "Goodbye\n");
	return;
}

module_init(rpisystimer_minit);
module_exit(rpisystimer_mcleanup);
