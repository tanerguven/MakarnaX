D = ../..
include $(D)/Makefile.inc
include ../MakarnaXMakefile

CFLAGS = -nostdinc -O0 -g3 -Wall \
	${INCLUDES} \
	-I$(D)/include

LDFLAGS = -static -nostdlib --L$(R)/lib

OBJS = \
	syscall.o \
	sys/stat.o \

all: crt0.o libmakarnax

libmakarnax: $(OBJS)
	i686-makarnax-ar cr libmakarnax.a $(OBJS)

clean:
	rm -f *.o *.a *.d
	cd sys; rm -f *.o *.a *.d