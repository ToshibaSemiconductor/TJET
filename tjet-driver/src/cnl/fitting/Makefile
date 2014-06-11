# add configuration if needed.
EXTRA_CFLAGS = $(JET_CFLAGS)


EXTRA_SYMVERS = $(JET_SRC_DIR)/$(JET_CMOS_BLD_DIR)/Module.symvers


obj-m                     := $(JET_FIT__DRV_NAME).o
$(JET_FIT__DRV_NAME)-objs  = cnlfit.o cnlfit_km.o cnlfit_cnl.o


all: $(JET_FIT__DRV_NAME).ko


$(JET_FIT__DRV_NAME).ko: cnlfit.c cnlfit_km.c cnlfit_cnl.c
	$(MAKE) -C $(KERNELDIR) KBUILD_EXTRA_SYMBOLS=$(EXTRA_SYMVERS) M=$(PWD) 	V=1 modules


clean:
	rm -rf *.o *~ core .depend .*.cmd *.ko *.mod.c .tmp_versions Module.symvers Module.markers modules.order


