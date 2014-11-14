#CFLAGS = -Wall -c -I /usr/include/SDL
#LDFLAGS = -lrt -lpthread -lSDL

CFLAGS = -Wall -c -D_NO_SDL_ -I /usr/include/SDL
LDFLAGS = -lrt -lpthread 

INSTALL = 

CFLAGS += $(EXTRA_CFLAGS)
LDFLAGS += $(EXTRA_LDFLAGS)
INSTALL += $(EXTRA_INSTALL)

PROG := $(shell basename `pwd`)

objects := $(patsubst %.c,%.o,$(wildcard *.c))

%.o: %.c
	$(CC) $(CFLAGS) -o $@ $< 

all: $(objects)
	gcc -o $(INSTALL)$(PROG) $(objects) $(LDFLAGS)

clean:
	rm -f $(objects) $(INSTALL)$(PROG)

mrproper:

	rm -rf *.ko *.o *.mod.c .*.cmd Module.symvers modules.order .tmp_versions $(LOCAL_OBJ)

