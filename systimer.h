#ifndef __SYSTIMER_H__
#define __SYSTIMER_H__
//magical IOCTL number
#define SYSTIMER_IOC_MAGIC 'k'

#define SYSTIMER_READ		_IOR(SYSTIMER_IOC_MAGIC, 0x80, unsigned long long int)
#define SYSTIMER_DELAY		_IOW(SYSTIMER_IOC_MAGIC, 0x81, unsigned long int)

#define SYSTIMER_LL_FIRST	_IOR(SYSTIMER_IOC_MAGIC, 0x82, unsigned int)
#define SYSTIMER_ll_PRINT	_IO(SYSTIMER_IOC_MAGIC, 0x83)
#define SYSTIMER_ll_INS		_IOW(SYSTIMER_IOC_MAGIC, 0x84, unsigned long int)
#define SYSTIMER_ll_DEL		_IOW(SYSTIMER_IOC_MAGIC, 0x85, unsigned long int)

//The header file <asm/ioctl.h>, which is included by <linux/ioctl.h>, defines
//macros that help set up the command numbers as follows:
// _IO(type,nr) (for a command that has no argument),
// _IOR(type,nr,datatype) (for reading data from the driver),
// _IOW(type,nr,datatype) (for writing data to the driver),
// _IOWR(type,nr,datatype) (for bidirectional transfers). The type and number
// 		fields are passed as arguments, and the size field is derived by applying sizeof to the datatype argument.



#endif //__SYSTIMER_H__
