D = ../..
include $(D)/Makefile.in
include ../MakarnaXMakefile

CFLAGS = -nostdinc -O0 -g3 -Wall \
	${INCLUDES} \
	-I$(D)/include

LDFLAGS = -static -nostdlib --L$(R)/lib

OBJS = \
	sys/sys.o \
	sys/stat.o \
	sys/brk.o \
	sys/process.o \
	sys/file.o \
	sys/inode.o \
	sys/signal.o \
	sys/sleep.o \
	sys/wait.o \
	sys/unistd.o \

all: crt0.o libmakarnax

libmakarnax: $(OBJS)
	i686-makarnax-ar cr libmakarnax.a $(OBJS)

syslib:
	ar r ../../tools/gcc_cross_compiler/local/i686-makarnax/lib/libc.a sys/*.o
	cp crt0.o ../../tools/gcc_cross_compiler/local/i686-makarnax/lib

clean:
	rm -f *.o *.a *.d
	cd sys; rm -f *.o *.a *.d
