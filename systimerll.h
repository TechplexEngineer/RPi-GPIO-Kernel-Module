#ifndef __STLL_H__
#define __STLL_H__

#include <linux/kernel.h>	//printk
#include <linux/slab.h>		//spinlock

#define CACHE_SIZE 25 //max number of linked list nodes

//Global variables:
struct stll_data {
	struct stll *st_timell; 					//pointer to linked list head
	struct kmem_cache *ll_kcache;
	spinlock_t st_lock;							//spinlock init in module init
};

//this is defined in systimerll.c
extern struct stll_data stlld; //stlld: SysTimer Linked List Data


//linked list node
struct stll {
	uint64_t t;									//data member
	struct stll *prev;							//previous node
	struct stll *next;							//nect node
};

struct stll *stll_first(struct stll *l);
void 		 stll_print(struct stll *l);
struct stll *stll_insert(struct stll *l, uint32_t t);
struct stll *stll_delete(struct stll *l, uint32_t t);

struct stll *stll_init(void);
void 		 stll_free(struct stll *l);

#endif //__STLL_H__