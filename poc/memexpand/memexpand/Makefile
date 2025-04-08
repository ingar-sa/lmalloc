PWD := $(shell pwd)
KDIR := /lib/modules/$(shell uname -r)/build
obj-m += memexpand.o

all:
	make -C $(KDIR) M=$(PWD) modules
	gcc main.c -o main
clean:
	make -C $(KDIR) M=$(PWD) clean
