CFLAGS = -Wall 
LDFLAGS = 
INSTALL = 

CFLAGS += $(EXTRA_CFLAGS)
LDFLAGS += $(EXTRA_LDFLAGS)
INSTALL += $(EXTRA_INSTALL)

PROG := $(shell basename `pwd`)

objects := $(patsubst %.c,%.exe,$(wildcard *.c))

all: $(objects)

%.exe: %.c
	$(CC) $(CFLAGS) -o $@ $< 

clean:
	rm -f $(objects) $(INSTALL)$(PROG)

mrproper:

