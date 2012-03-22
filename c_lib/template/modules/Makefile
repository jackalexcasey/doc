PWD   := $(shell pwd)
KDIR  := /lib/modules/$(shell uname -r)/build
NAME := template

obj-m := $(NAME).o
$(NAME)-objs := main.o

default:
	$(MAKE) -C $(KDIR) M=$(PWD) modules

clean:
	rm -rf *.ko *.o *.mod.c .*.cmd Module.symvers modules.order .tmp_versions 
