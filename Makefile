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

clean-kernel:
	cd kernel; make clean;

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

all-tools:
	cd tools/gcc_cross_compiler; make
	ln -f -s ${CURDIR}/user_programs/lib/libmakarnax.a tools/gcc_cross_compiler/local/i686-makarnax/lib/.
	ln -f -s ${CURDIR}/user_programs/lib/crt0.o tools/gcc_cross_compiler/local/i686-makarnax/lib/.

clean-tools:
	cd tools/gcc_cross_compiler; make clean;

clean:
	@for p in  $(DIRS); \
	do \
		cd $$p; make clean; cd ..; \
	done
	rm -f bin/kernel bin/kernel.asm;

clean-test:
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
