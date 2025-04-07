PWD := $(shell pwd)
KDIR := /lib/modules/$(shell uname -r)/build
obj-m += my_device.o

all:
	make -C $(KDIR) M=$(PWD) modules
	gcc main.c -o main
