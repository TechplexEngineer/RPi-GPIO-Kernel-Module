#ifndef __RPI_GPIO_H__
#define __RPI_GPIO_H__
//magical IOCTL number
#define SYSTIMER_IOC_MAGIC 'k'

#define GPIO_READ			_IOR(SYSTIMER_IOC_MAGIC, 0x90, unsigned char) //in: pin to read				//out: value 			//the value read the value
#define GPIO_WRITE			_IOW(SYSTIMER_IOC_MAGIC, 0x90, unsigned char) //in: struct(pin, data)		//out: NONE

#define GPIO_REQUEST		_IOR(SYSTIMER_IOC_MAGIC, 0x90, unsigned char) //in: pin to reque			//out: success/fail 	// request exclusive write privalages
#define GPIO_FREE			_IOR(SYSTIMER_IOC_MAGIC, 0x90, unsigned char) //in: pin to free 			//out: success/fail?

#define GPIO_TOGGLE			_IOW(SYSTIMER_IOC_MAGIC, 0x90, unsigned char) //in: pin to toggle			//out: NONE
#define GPIO_MODE			_IOW(SYSTIMER_IOC_MAGIC, 0x90, unsigned char) //in: struct (pin, mode[i/o])




//The header file <asm/ioctl.h>, which is included by <linux/ioctl.h>, defines
//macros that help set up the command numbers as follows:
// _IO(type,nr) (for a command that has no argument),
// _IOR(type,nr,datatype) (for reading data from the driver),
// _IOW(type,nr,datatype) (for writing data to the driver),
// _IOWR(type,nr,datatype) (for bidirectional transfers). The type and number
// 		fields are passed as arguments, and the size field is derived by applying sizeof to the datatype argument.

//https://github.com/raspberrypi/linux/blob/rpi-3.6.y/Documentation/ioctl/ioctl-number.txt


#endif //__RPI_GPIO_H__
