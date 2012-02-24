D = ../..
include $(D)/Makefile.inc

CFLAGS = $(M) -I$(D)/include -O1 -g3 -Wall -Wno-unused-but-set-variable -c \
	 -nostdlib -fno-builtin -fno-stack-protector -Isysdeps/i386/ -I./ -Isysdeps/generic

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
	@echo [ar] string.a
	@ar cr string.a $(OBJS)
	@echo [ld] string.O
	@$(LD) -r -o string.O $(OBJS)

clean:
	rm -f *.o *.O *.a;
