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

#include <linux/cdev.h>
#include <linux/slab.h>

#include "systimerll.h"

struct stll_data stlld = { //stlld: SysTimer Linked List Data
	.st_timell = NULL,
};

// Get the first element in the list
struct stll *stll_first(struct stll *l)
{
	spin_lock(&stlld.st_lock);
	while (l->prev!=NULL) {
		l=l->prev;
	}
	spin_unlock(&stlld.st_lock);
	return l;
}

// Print all members
void stll_print(struct stll *l)
{
	struct stll *f=stll_first(l);

	printk(KERN_DEBUG "\n%10p | %10p %10p %llu\n",f, f->prev, f->next, f->t);
	spin_lock(&stlld.st_lock);
	while (f->next) {
		f=f->next;
		//this is bad because printk is complex, so we hold the lock a long time
		//copy f to a temporary variable
		printk(KERN_DEBUG "%10p | %10p %10p %llu\n",f, f->prev, f->next, f->t);
	}
	spin_unlock(&stlld.st_lock);
	return;
}

// Initialize a linked list
struct stll *stll_init(void)
{
	struct stll *l=NULL;

	// Create space
	//l=(struct stll *)malloc(sizeof(struct stll));
	l=(struct stll *)kmalloc(sizeof(struct stll), GFP_KERNEL);
	if (l==NULL) {
		//perror("malloc");
		return NULL;
	}
	// Initialize
	l->t=0;
	l->prev=NULL;
	l->next=NULL;
	return l;
}

// Insert a member to the list
// Return newly created member
struct stll *stll_insert(struct stll *l, uint32_t t)
{
	struct stll *c=stll_first(l);
	struct stll *f=NULL;

	if (t==0) return l;
	spin_lock(&stlld.st_lock);
	while (c) {
		if (t>(c->t)) {
			if (c->next) {	// middle of list
				if (t<=(c->next->t)) break;
			} else {		// end of list
				break;
			}
		}
		c=c->next;
	}
	spin_unlock(&stlld.st_lock);
	// Add new AFTER c
	//f=(struct stll *)malloc(sizeof(struct stll));
	f=(struct stll *)kmalloc(sizeof(struct stll), GFP_KERNEL);
	if (f==NULL) {
		printk(KERN_INFO "kmem cache alloc failed");
		//perror("malloc");
		return NULL;
	}
	f->t=t;
	f->prev=c;
	f->next=c->next;
	c->next=f;
	if (f->next) f->next->prev=f;
	return f;									//change this from c to f (the one we just created)
}

// Delete a member of the list
// based on the value
struct stll *stll_delete(struct stll *l, uint32_t t)
{
	struct stll *c=stll_first(l);
	struct stll *p=NULL;						//so deletion can be outside lock

	if (t==0) return l;	// Not a possible time
	spin_lock(&stlld.st_lock);
	while (c) {
		if (t==(c->t)) break;
		c=c->next;
		p=c;
	}
	if (!c) return NULL;	// Entry not found
	if (c->prev==NULL) return l;	// Cannot remove the first one
	// Remove the current entry c
	if (c->next) {	// Middle of list
		c->prev->next=c->next;
		c->next->prev=c->prev;
	} else {	// end of list
		c->prev->next=NULL;
	}
	spin_unlock(&stlld.st_lock);
	kfree(p);
	return l;									//changed from c(dangling pinter) to l(start of the list)
}

// Free all list element memory
void stll_free(struct stll *l)
{
	struct stll *c=stll_first(l);

	while (c->next) {
		c=c->next;
		kfree(c->prev);	//the last element isn't destroyed
	}
	kfree(c);
	return;
}