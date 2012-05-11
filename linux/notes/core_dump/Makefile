CFLAGS = -Wall -c
LDFLAGS = -lrt
INSTALL = /nobackup/lucid/chroot/root/prog/

CFLAGS += $(EXTRA_CFLAGS)
LDFLAGS += $(EXTRA_LDFLAGS)
INSTALL += $(EXTRA_INSTALL)

PROG := $(shell basename `pwd`)

objects := $(patsubst %.c,%.o,$(wildcard *.c))

%.o: %.c
	$(CC) $(CFLAGS) -o $@ $< 

all: $(objects)
	gcc -o $(PROG) $(objects) $(LDFLAGS)

install: $(PROG)
	cp $(PROG) $(INSTALL)

clean:
	rm -f $(objects) $(PROG)

mrproper:

