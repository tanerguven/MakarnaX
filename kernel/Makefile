D = ..
include $(D)/Makefile.inc

OBJS = \
	boot.o\
	init.o\
	console.o\
	keyboard.o\
	print.o\
	panic.o\
	kernel_monitor.o\
	memory/physical.o\
	memory/virtual.o\
	memory/memory.o\
	memory/kmalloc.o\
	task.o\
	task_exit.o\
	trap.o\
	traphandler.o\
	syscall.o\
	syscall_table.o\
	sched.o\
	picirq.o\
	timer.o\
	signal.o\
	spinlock.o\
	ipc/ipc.o\
	ipc/semaphore.o\
	ipc/shared_memory.o\
	fs/vfs.o\
	fs/superblock.o\
	fs/file.o\
	fs/dir.o\
	fs/denemefs/denemefs.o\
	fs/denemefs/file.o\
	fs/denemefs/dir.o\
	fs/exec.o\
	fs/devices.o\

DEPS = $(OBJS:.o=.d)

all: $(OBJS)
	@echo [ld] kernel.O
	@$(LD) -r -o kernel.O $(OBJS)

-include $(DEPS)

clean:
	find . -name "*.o" | xargs rm -f
	find . -name "*.d" | xargs rm -f
	rm -f kernel.O
