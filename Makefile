
obj-m += systimer.o
systimer-objs := systimerll.o systimercore.o

all:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) modules

clean: bclean


bclean:
	rm -f .*.cmd *.o *.order *.mod.c *.ko

kclean:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) clean