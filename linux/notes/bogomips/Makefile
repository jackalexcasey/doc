KDIR  := /lib/modules/`uname -r`/build
#KDIR := /nobackup/gitrepo/third
#KDIR := /nobackup/lucid/gitrepo/WRL
LOCAL_OBJ := bogomips.o
obj-m := $(LOCAL_OBJ)

default:
	$(MAKE) V=1 -C $(KDIR) M=$(PWD) modules

clean:
	rm -rf *.ko *.o *.mod.c .*.cmd Module.symvers modules.order .tmp_versions $(LOCAL_OBJ) *.csv


