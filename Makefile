# obj-m specifie we're a kernel module.
obj-m += stdout.o

# Set the path to the Kernel build utils.
KBUILD=/lib/modules/$(shell uname -r)/build/
 
default:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) modules

clean:
	rm *.mod *.mod.c *.o *.symvers *.order *.ko
