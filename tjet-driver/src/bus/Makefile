# add configuration if needed.
EXTRA_CFLAGS = $(JET_CFLAGS) \
	-I$(JET_TOP_DIR)/../sdioapi/core


EXTRA_SYMVERS = \
	$(JET_TOP_DIR)/../sdioapi/Module.symvers \
	$(JET_SRC_DIR)/$(JET_CMOS_BLD_DIR)/Module.symvers \


obj-m                     := $(JET_BUS__DRV_NAME).o
$(JET_BUS__DRV_NAME)-objs  = core/buscmn.o sdio/bus_sdio.o



all: $(JET_BUS__DRV_NAME).ko


$(JET_BUS__DRV_NAME).ko: core/buscmn.c sdio/bus_sdio.c
	cat $(EXTRA_SYMVERS) > $(JET_SRC_DIR)/$(JET_BUS__BLD_DIR)/Module.symvers
	$(MAKE) -C $(KERNELDIR) M=$(PWD) V=1 modules


clean:
	rm -rf *.o */*.o *~ */*~ .depend .*.cmd */.*.cmd *.ko *.mod.c .tmp_versions Module.symvers Module.markers modules.order


