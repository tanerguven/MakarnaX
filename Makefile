include Makefile.in

DIRS = \
	kernel \
	lib \

OBJS = \
	kernel/kernel.O \
	lib/lib.O \
	lib/string/string.O \

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

rm_objs:
	@rm -f $(OBJS)

kernel: dirs
	@mkdir -p bin
	@echo [ld] bin/kernel
	@$(LD) -Tscripts/link.ld -o"bin/kernel" $(OBJS) $(LIBS)
	@objdump -S bin/kernel > bin/kernel.asm

clean-kernel:
	cd kernel; make clean;

usr_lib:
	cd user_programs/lib; make clean; make; make syslib;

usr_bin:
	cd user_programs/bin; make clean; make;

user:
	cd user_programs; make;

clean-user:
	cd user_programs; make clean;

initrd:
	cp -f init_programs bin/initrd
	cd bin; tar cf initrd.tar initrd

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

ktest-%: ktest-clean
	make DEFS=-DKTEST=$*

ktest-clean:
	rm -f kernel/init.o

documentation:
	doxygen doc/doc.doxygen

documentation-clean:
	rm -rf doc/html

######################################
#		running
######################################

tmp-dir:
	rm -rf tmp
	mkdir tmp

qemu: tmp-dir kernel #image
	$(QEMU) -kernel bin/kernel -initrd bin/initrd.tar -serial mon:stdio -serial file:tmp/serial2

qemu-gdb: kernel #image
	$(QEMU) -kernel bin/kernel -serial mon:stdio -S -s
