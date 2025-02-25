ARCH	= linux

CC	= gcc
CFLAGS	= -g -O2 

MACHDEP_OBJS= getopt.o
MACHDEP_LIBS=

all: floorpln.udp

include common

getopt.o: $(TmkLIB)/support/getopt.c
	$(CC) $(CFLAGS) -c $(TmkLIB)/support/getopt.c
