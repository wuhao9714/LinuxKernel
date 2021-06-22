oops-obj:=route_struct.o
obj-m:=route_struct.o

all:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) modules;
clean:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) clean;

