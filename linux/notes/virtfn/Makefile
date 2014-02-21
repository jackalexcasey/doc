KDIR  := /lib/modules/`uname -r`/build
#KDIR := /nobackup/precise/chroot/usr/src/linux-3.4.x
#KDIR := /usr/src/linux-3.4.x
LOCAL_OBJ := virtfn.o client.o
obj-m := $(LOCAL_OBJ)

default:
	$(MAKE) V=1 -C $(KDIR) M=$(PWD) modules

clean:
	rm -rf *.ko *.o *.mod.c .*.cmd Module.symvers modules.order .tmp_versions $(LOCAL_OBJ) *.csv


