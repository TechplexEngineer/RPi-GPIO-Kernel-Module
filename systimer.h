#ifndef __SYSTIMER_H__
#define __SYSTIMER_H__

#define SYSTIMER_IOC_MAGIC 'k'

#define SYSTIMER_READ _IOR(SYSTIMER_IOC_MAGIC, 0x80, unsigned long int)

#endif //__SYSTIMER_H__
