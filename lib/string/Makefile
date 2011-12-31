D = ../..
include $(D)/Makefile.inc

CFLAGS = -I$(D)/include -O1 -g3 -Wall -c -nostdlib -fno-builtin -fno-stack-protector
CFLAGS += -Isysdeps/i386/ -I./ -Isysdeps/generic

CPPFLAGS = $(CFLAGS)

OBJS = memcpy.o \
	memmove.o \
	memset.o \
	strcat.o \
	strcmp.o \
	strcpy.o \
	strlen.o \
	strncat.o \
	strncmp.o \
	strncpy.o \
	strnlen.o \
	memcmp.o \

all: $(OBJS)
	ar cr string.a $(OBJS)
	$(LD) -r -o string.O $(OBJS)

clean:
	rm -f *.o *.O *.a;
