ifneq	($(KERNELRELEASE),)
obj-m	:= tasklist.o 

else
KDIR	:= /lib/modules/$(shell uname -r)/build
default:	
	make -C $(KDIR) M=$(PWD) modules 
	rm -r -f .tmp_versions *.mod.c .*.cmd *.o *.symvers 
clean:
	make -C $(KDIR) M=$(PWD) clean;

endif
