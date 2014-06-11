# add configuration if needed.
EXTRA_CFLAGS = $(JET_CFLAGS) \


EXTRA_SYMVERS = \
	$(JET_SRC_DIR)/$(JET_CMOS_BLD_DIR)/Module.symvers \
	$(JET_SRC_DIR)/$(JET_BUS__BLD_DIR)/Module.symvers \
	$(JET_SRC_DIR)/$(JET_FIT__BLD_DIR)/Module.symvers \


#KERNELDIR ?= /lib/modules/$(shell uname -r)/build




obj-m                     := $(JET_CNL__DRV_NAME).o
$(JET_CNL__DRV_NAME)-objs  = cnl.o cnl_schd.o cnl_izan.o cnl_if.o cnl_sm.o cnl_km.o cnl_task.o cnl_util.o




all: $(JET_CNL__DRV_NAME).ko


$(JET_CNL__DRV_NAME).ko: cnl.c cnl_schd.c cnl_izan.c cnl_if.c cnl_sm.c cnl_km.c cnl_task.c cnl_util.c
	cat $(EXTRA_SYMVERS) > $(JET_SRC_DIR)/$(JET_CNL__BLD_DIR)/Module.symvers
	$(MAKE) -C $(KERNELDIR) M=$(PWD) V=1 modules


clean:
	rm -rf *.o *~ .depend .*.cmd *.ko *.mod.c .tmp_versions Module.symvers Module.markers modules.order


