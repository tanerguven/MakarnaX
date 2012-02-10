D = ..
include ../Makefile.inc

CFLAGS = $(M) -I../include -O0 -g3 -Wall -c -nostdlib -nostdinc -fno-builtin\
	    -fno-stack-protector -nostartfiles -nodefaultlibs -D_USER_PROGRAM

CXXFLAGS = $(CFLAGS) -fno-exceptions -fno-rtti

LIBS = \
	../lib/user_lib.a \

PROGRAMS = \
	divide_error.bin \
	hello.bin \
	yield.bin \
	forktest.bin \
	dongu.bin \
	sys_dongu.bin \
	signaltest.bin \
	init.bin \
	ipctest.bin \
	processmemtest.bin \
	kill.bin \

all: $(PROGRAMS)

test.bin: test.o
	$(LD) -T"link.ld" -o test.bin test.o $(LIBS)

hello.bin: hello.o
	$(LD) -T"link.ld" -o hello.bin hello.o $(LIBS)

divide_error.bin: divide_error.o
	$(LD) -T"link.ld" -o divide_error.bin divide_error.o $(LIBS)

yield.bin: yield.o
	$(LD) -T"link.ld" -o yield.bin yield.o $(LIBS)

forktest.bin: forktest.o
	$(LD) -T"link.ld" -o forktest.bin forktest.o $(LIBS)

dongu.bin: dongu.o
	$(LD) -T"link.ld" -o dongu.bin dongu.o $(LIBS)

sys_dongu.bin: sys_dongu.o
	$(LD) -T"link.ld" -o sys_dongu.bin sys_dongu.o $(LIBS)

signaltest.bin: signaltest.o
	$(LD) -T"link.ld" -o signaltest.bin signaltest.o $(LIBS)

init.bin: init.o
	$(LD) -T"link.ld" -o init.bin init.o $(LIBS)

ipctest.bin: ipctest.o
	$(LD) -T"link.ld" -o ipctest.bin ipctest.o $(LIBS)

processmemtest.bin: processmemtest.o
	$(LD) -T"link.ld" -o processmemtest.bin processmemtest.o $(LIBS)

kill.bin: kill.o
	$(LD) -T"link.ld" -o kill.bin kill.o $(LIBS)

clean:
	rm -f *.o *.bin
	cd linux; make clean;
