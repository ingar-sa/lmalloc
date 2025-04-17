PWD := $(shell pwd)
KDIR := /lib/modules/$(shell uname -r)/build
BUILD_DIR := $(PWD)/build

obj-m += memexpand.o

all: build-dir kernel-module userspace
	mv memexpand.ko $(BUILD_DIR)/memexpand
	make -C $(KDIR) M=$(PWD) clean
	mv $(BUILD_DIR)/memexpand $(BUILD_DIR)/memexpand.ko
	
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
