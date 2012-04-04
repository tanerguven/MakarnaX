D = ../..
include $(D)/Makefile.inc

R = $(D)/tools/gcc_cross_compiler/local
PATH := ${PATH}:$(R)/bin
CC = i686-makarnax-gcc
GCC = i686-makarnax-gcc
LD = i686-makarnax-gcc


CFLAGS = -nostdinc -O0 -g3 -Wall \
	-I$(R)/include \
	-I$(R)/lib/gcc/i686-makarnax/4.6.3/include \
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