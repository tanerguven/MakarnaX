D = ../..
include $(D)/Makefile.inc

CFLAGS = $(M) -I$(D)/include -Iinclude -O0 -g3 -Wall -c -nostdlib -nostdinc -fno-builtin\
	    -fno-stack-protector -nostartfiles -nodefaultlibs -D_USER_PROGRAM

CXXFLAGS = $(CFLAGS) -fno-exceptions -fno-rtti

LIBS = \
	$(D)/lib/user_lib.a \

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
	fs.bin \
	cross_compile_error.bin 

all: $(PROGRAMS)

test.bin: test.o
	@echo [ld] test.bin
	@$(LD) -T"link.ld" -o test.bin test.o $(LIBS)

hello.bin: hello.o
	@echo [ld] test.bin
	@$(LD) -T"link.ld" -o hello.bin hello.o $(LIBS)

cross_compile_error.bin: cross_compile_error.o
	@echo [ld] cross_compile_error.bin
	@$(LD) -T"link.ld" -o cross_compile_error.bin cross_compile_error.o $(LIBS)

divide_error.bin: divide_error.o
	@echo [ld] divide_error.bin
	@$(LD) -T"link.ld" -o divide_error.bin divide_error.o $(LIBS)

yield.bin: yield.o
	@echo [ld] yield.bin
	@$(LD) -T"link.ld" -o yield.bin yield.o $(LIBS)

forktest.bin: forktest.o
	@echo [ld] forktest.bin
	@$(LD) -T"link.ld" -o forktest.bin forktest.o $(LIBS)

dongu.bin: dongu.o
	@echo [ld] dongu.bin
	@$(LD) -T"link.ld" -o dongu.bin dongu.o $(LIBS)

sys_dongu.bin: sys_dongu.o
	@echo [ld] sys_dongu.bin
	@$(LD) -T"link.ld" -o sys_dongu.bin sys_dongu.o $(LIBS)

signaltest.bin: signaltest.o
	@echo [ld] signaltest.bin
	@$(LD) -T"link.ld" -o signaltest.bin signaltest.o $(LIBS)

init.bin: init.o
	@echo [ld] init.bin
	@$(LD) -T"link.ld" -o init.bin init.o $(LIBS)

ipctest.bin: ipctest.o
	@echo [ld] ipctest.bin
	@$(LD) -T"link.ld" -o ipctest.bin ipctest.o $(LIBS)

processmemtest.bin: processmemtest.o
	@echo [ld] processmemtest.bin
	@$(LD) -T"link.ld" -o processmemtest.bin processmemtest.o $(LIBS)

kill.bin: kill.o
	@echo [ld] kill.bin
	@$(LD) -T"link.ld" -o kill.bin kill.o $(LIBS)

fs.bin: fs.o
	@echo [ld] fs.bin
	@$(LD) -T"link.ld" -o fs.bin fs.o $(LIBS)

clean:
	rm -f *.o *.bin *.d
	cd linux; make clean;
