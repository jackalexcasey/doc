KDIR  := /nobackup/lucid64/gitrepo/third
PWD   := $(shell pwd)
EXTRA_CFLAGS := -I$(TOP)/include
LOCAL_OBJ := bogomips.o

obj-m := $(LOCAL_OBJ)

default:
	$(MAKE) -C $(KDIR) M=$(PWD) modules

install:
	$(MAKE) -C $(KDIR) M=$(PWD) INSTALL_MOD_PATH=$(INSTALL_PATH) modules_install

clean:
	rm -rf *.ko *.o *.mod.c .*.cmd Module.symvers modules.order .tmp_versions $(LOCAL_OBJ)
