CC=	cc

CFLAGS=	-O -std=gnu99 -Wall
LIBS=	-lpthread -lrt

PROG=	bogomips

SRCS=	bogomips.c cpuset.c threads.c logging.c clock.c
HDRS=	spinlock.h cpuset.h threads.h logging.h clock.h
OBJS=	$(SRCS:.c=.o)

all:	$(PROG)

$(PROG):	$(OBJS)
	$(CC) $(LDFLAGS) -o $(PROG) $(OBJS) $(LIBS)

$(OBJS):	$(HDRS)

clean:
	-rm -f $(OBJS) out.* $(PROG) nr_cpu*

clobber:	clean
	-rm -f $(PROG)
