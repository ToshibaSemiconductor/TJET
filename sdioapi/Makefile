export KERNELDIR ?= 

export ARCH=arm
export CROSS_COMPILE=arm-eabi-


ifneq ($(KERNELRELEASE),)
	obj-m += sdio.o core/
else
	KERNELDIR ?= /usr/src/linux
	PWD := $(shell pwd)

all:	default
	mkdir -p objs/
	cp sdio.ko core/sdiocore.ko objs/
	ls -l objs/*

default:
	$(MAKE) -C $(KERNELDIR) M=$(PWD) modules

clean:
	$(MAKE) -C $(KERNELDIR) M=$(PWD) clean
	rm -rf objs

endif
