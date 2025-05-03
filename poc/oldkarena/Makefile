PWD := $(shell pwd)
KDIR := /lib/modules/$(shell uname -r)/build
BUILD_DIR := $(PWD)/build

obj-m += okarena.o

all: build-dir kernel-module userspace
	mv okarena.ko $(BUILD_DIR)/karena
	make -C $(KDIR) M=$(PWD) clean
	mv $(BUILD_DIR)/karena $(BUILD_DIR)/okarena.ko
	
build-dir:
	mkdir -p $(BUILD_DIR)

kernel-module:
	make -C $(KDIR) M=$(PWD) modules

userspace:
	gcc main.c -o $(BUILD_DIR)/main

clean:
	make -C $(KDIR) M=$(PWD) clean
	rm -f $(BUILD_DIR)/main
	rm -rf $(BUILD_DIR)

.PHONY: all build-dir kernel-module userspace clean
