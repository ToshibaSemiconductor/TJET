# add configuration if needed.
EXTRA_CFLAGS = $(JET_CFLAGS)

EXTRA_SYMVERS = $(JET_SRC_DIR)/$(JET_FIT__BLD_DIR)/Module.symvers


obj-m                     := $(JET_IOFT_DRV_NAME).o
$(JET_IOFT_DRV_NAME)-objs  = cnlio_fit.o


all: $(JET_IOFT_DRV_NAME).ko


$(JET_IOFT_DRV_NAME).ko: cnlio_fit.c
	$(MAKE) -C $(KERNELDIR) KBUILD_EXTRA_SYMBOLS=$(EXTRA_SYMVERS) M=$(PWD) 	V=1 modules


clean:
	rm -rf *.o *~ core .depend .*.cmd *.ko *.mod.c .tmp_versions Module.symvers Module.markers modules.order


