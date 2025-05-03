PWD := $(shell pwd)
KDIR := /lib/modules/$(shell uname -r)/build
BUILD_DIR := $(PWD)/build

LIBS =  -lpthread -lpq -lm
INCLUDES = -I. 

obj-m += karena.o

all: build-dir kernel-module userspace
	mv karena.ko $(BUILD_DIR)/karena
	make -C $(KDIR) M=$(PWD) clean
	mv $(BUILD_DIR)/karena $(BUILD_DIR)/karena.ko
	
build-dir:
	mkdir -p $(BUILD_DIR)

kernel-module:
	make -C $(KDIR) M=$(PWD) modules

userspace:
	gcc main.c $(INCLUDES) -o $(BUILD_DIR)/main $(LIBS)

clean:
	make -C $(KDIR) M=$(PWD) clean
	rm -f $(BUILD_DIR)/main
	rm -rf $(BUILD_DIR)

local:
	make -C $(shell nix-build -E '(import <nixpkgs> {}).linux.dev' --no-out-link)/lib/modules/*/build M=$(pwd) modules

.PHONY: all build-dir kernel-module userspace clean
