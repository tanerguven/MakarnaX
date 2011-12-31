D = ..
include $(D)/Makefile.inc

OBJS = \
	boot.o\
	init.o\
	console.o\
	keyboard.o\
	printf.o\
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
	sched.o\
	picirq.o\
	timer.o\
	signal.o\
	ipc/ipc.o\
	ipc/semaphore.o\
	ipc/shared_memory.o\

all: $(OBJS)
	$(LD) -r -o kernel.O $(OBJS)

clean:
	rm -f *.o memory/*.o ipc/*.o;
	rm -f kernel.O;