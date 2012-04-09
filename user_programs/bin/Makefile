D = ../..
include $(D)/Makefile.inc

R = $(D)/tools/gcc_cross_compiler/local
PATH := ${PATH}:$(R)/bin
CC = i686-makarnax-gcc
GCC = i686-makarnax-gcc
LD = i686-makarnax-gcc

CFLAGS = -nostdinc -O0 -g3 -Wall -Werror=implicit-function-declaration \
	-I$(R)/include \
	-I$(R)/lib/gcc/i686-makarnax/4.6.3/include

USER_LIBS = -lc -lmakarnax
LDFLAGS = -static -L$(R)/lib

PROGRAMS = \
	hello_newlib.bin \

all: $(PROGRAMS)

hello_newlib.bin: hello_newlib.o
	@echo [$(LD)] hello_newlib.bin
	@$(LD) $(LDFLAGS) -o hello_newlib.bin hello_newlib.o $(USER_LIBS)
	rm -f ../test/hello_newlib.bin
	ln -s `pwd`/hello_newlib.bin ../test/hello_newlib.bin

clean:
	rm -f *.o *.bin *.d