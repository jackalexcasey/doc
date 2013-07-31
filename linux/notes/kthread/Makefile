KDIR := /nobackup/gitrepo/third
LOCAL_OBJ := kthread.o
obj-m := $(LOCAL_OBJ)

default:
	$(MAKE) V=1 -C $(KDIR) M=$(PWD) modules

clean:
	rm -rf *.ko *.o *.mod.c .*.cmd Module.symvers modules.order .tmp_versions $(LOCAL_OBJ) *.csv


