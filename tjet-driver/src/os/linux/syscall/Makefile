# add configuration if needed.
EXTRA_CFLAGS = $(JET_CFLAGS)


obj-m                     := $(JET_CMOS_DRV_NAME).o
$(JET_CMOS_DRV_NAME)-objs  = cmn_tsk.o cmn_mem.o cmn_sync.o cmn_lock.o cmn_time.o cmn_msg.o cmn_util.o cmn_pwrlock.o oscmn.o 


all: $(JET_CMOS_DRV_NAME).ko


$(JET_CMOS_DRV_NAME).ko: cmn_tsk.c cmn_mem.c cmn_sync.c cmn_lock.c cmn_time.c cmn_msg.c cmn_util.c cmn_pwrlock.c oscmn.c
	$(MAKE) -C $(KERNELDIR) M=$(PWD) V=1 modules


clean:
	rm -rf *.o *~ .depend .*.cmd *.ko *.mod.c .tmp_versions Module.symvers Module.markers modules.order


