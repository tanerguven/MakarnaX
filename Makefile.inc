#$(D) -> ".." "../.."

CC = gcc -m32
ASFLAGS = -I$(D)/include
CFLAGS = -I$(D)/include -O1 -g3 -Wall -c -nostdlib -nostdinc -fno-builtin\
	    -fno-stack-protector -nostartfiles -nodefaultlibs $(KERNEL_CFLAGS)\
		$(DEFS)
CXX = $(CC)
CXXFLAGS = $(CFLAGS) -fno-exceptions -fno-rtti
AS = gcc -m32
LD = ld -m elf_i386