CFLAGS = -Wall -c
LDFLAGS = -lrt
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

