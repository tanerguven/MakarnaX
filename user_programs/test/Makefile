D = ../..
include $(D)/Makefile.in

CFLAGS = $(M) -I$(D)/include -Iinclude -O0 -g3 -Wall -c -nostdlib -nostdinc -fno-builtin\
	    -fno-stack-protector -nostartfiles -nodefaultlibs -D_USER_PROGRAM

CXXFLAGS = $(CFLAGS) -fno-exceptions -fno-rtti

LIBS = \
	$(D)/lib/user_lib.a

PROGRAMS = \
	bin/divide_error \
	bin/hello \
	bin/yield \
	bin/forktest \
	bin/dongu \
	bin/sys_dongu \
	bin/signaltest \
	bin/init \
	bin/ipctest \
	bin/processmemtest \
	bin/kill \
	bin/fs \
	bin/cat

all: $(PROGRAMS)

# yeni olusturulan object dosyalarinin compile sonunda silinmemesi icin
.SECONDARY:

bin/%: %.o
	@echo [ld] $*
	@$(LD) -T"link.ld" -o $(INITRD_BIN_FOLDER)/$* $*.o $(LIBS)

clean:
	rm -f *.o *.bin *.d
	cd linux; make clean;
