#KDIR  := /lib/modules/`uname -r`/build
#KDIR := /nobackup/gitrepo/third
KDIR := /nobackup/gitrepo/WRL
LOCAL_OBJ := pci_rescan.o
obj-m := $(LOCAL_OBJ)

default:
	$(MAKE) -C $(KDIR) M=$(PWD) modules

clean:
	rm -rf *.ko *.o *.mod.c .*.cmd Module.symvers modules.order .tmp_versions $(LOCAL_OBJ) *.csv


