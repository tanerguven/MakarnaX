include Makefile.inc

DIRS = \
	kernel \
	lib \
	user_programs \

OBJS = \
	kernel/kernel.O \
	lib/lib.O \
	lib/string/string.O \

PROGRAMS = \
	user_programs/test/divide_error.bin \
	user_programs/test/hello.bin \
	user_programs/test/yield.bin \
	user_programs/test/forktest.bin \
	user_programs/test/dongu.bin \
	user_programs/test/sys_dongu.bin \
	user_programs/test/signaltest.bin \
	user_programs/test/init.bin \
	user_programs/test/ipctest.bin \
	user_programs/test/processmemtest.bin \
	user_programs/test/kill.bin \
	user_programs/test/fs.bin \
	user_programs/test/hello_newlib.bin \

######################################
#	env variables
######################################

ifndef QEMU
QEMU := qemu
endif

######################################
#
######################################

all: rm_objs kernel

debug:
	export KERNEL_CFLAGS=-D__KERNEL_DEBUG__

rm_objs:
	@rm -f $(OBJS)

kernel: dirs
	@mkdir -p bin
	@echo [ld] bin/kernel
	@$(LD) -Tscripts/link.ld -o"bin/kernel" $(OBJS) $(LIBS) -b binary $(PROGRAMS) init_programs
	@objdump -S bin/kernel > bin/kernel.asm

usr_lib:
	cd user_programs/lib; make clean; make;

usr_bin:
	cd user_programs/bin; make clean; make;

user:
	cd user_programs; make clean; make;

linux:
	cd user_programs/test/linux; make;

dirs:
	@for p in  $(DIRS); \
    do \
	cd $$p; make; cd ..; \
    done

gcc_cross_compiler:
	cd tools/gcc_cross_compiler; ./build.sh;
	ln -f -s `pwd`/user_programs/lib/libmakarnax.a tools/gcc_cross_compiler/local/lib/.
	ln -f -s `pwd`/user_programs/lib/crt0.o tools/gcc_cross_compiler/local/lib/.
	rm -rf tools/gcc_cross_compiler/build

tools-clean:
	rm -f tools/gcc_cross_compiler/*.tar.*
	rm -rf tools/gcc_cross_compiler/build
	rm -rf tools/gcc_cross_compiler/local

clean:
	@for p in  $(DIRS); \
	do \
		cd $$p; make clean; cd ..; \
	done
	rm -f bin/kernel bin/kernel.asm;

clear-test:
	echo "" > init_programs

test-%:
	echo "/bin/"$* >> init_programs

documentation:
	doxygen doc/doc.doxygen

documentation-clean:
	rm -rf doc/html

######################################
#		running
######################################

qemu: kernel #image
	$(QEMU) -kernel bin/kernel -serial mon:stdio

qemu-gdb: kernel #image
	$(QEMU) -kernel bin/kernel -serial mon:stdio -S -s
