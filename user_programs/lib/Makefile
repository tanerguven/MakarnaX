D = ../..
include $(D)/Makefile.inc
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

all: crt0.o libmakarnax

libmakarnax: $(OBJS)
	i686-makarnax-ar cr libmakarnax.a $(OBJS)

clean:
	rm -f *.o *.a *.d
	cd sys; rm -f *.o *.a *.d