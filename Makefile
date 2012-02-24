include Makefile.inc

DIRS = kernel lib user_programs

OBJS = \
	kernel/kernel.O \
	lib/lib.O \
	lib/string/string.O \

PROGRAMS = \
	user_programs/divide_error.bin \
	user_programs/hello.bin \
	user_programs/yield.bin \
	user_programs/forktest.bin \
	user_programs/dongu.bin \
	user_programs/sys_dongu.bin \
	user_programs/signaltest.bin \
	user_programs/init.bin \
	user_programs/ipctest.bin \
	user_programs/processmemtest.bin \
	user_programs/kill.bin \
	user_programs/fs.bin \

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

user:
	cd user_programs; make clean; make;

linux:
	cd user_programs/linux; make;

dirs:
	@for p in  $(DIRS); \
    do \
	cd $$p; make; cd ..; \
    done

clean:
	@for p in  $(DIRS); \
	do \
		cd $$p; make clean; cd ..; \
	done
	rm -f bin/kernel bin/kernel.asm;

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
