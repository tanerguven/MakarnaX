#$(D) -> ".." "../.."

M = -m32

ASFLAGS = $(M) -I$(D)/include

CFLAGS = $(M) -I$(D)/include -O1 -g3 -Wall -c -nostdlib -nostdinc -fno-builtin\
	    -fno-stack-protector -nostartfiles -nodefaultlibs $(KERNEL_CFLAGS)\
		$(DEFS)

CXXFLAGS = $(CFLAGS) -fno-exceptions -fno-rtti

LD = ld -m elf_i386

%.o: %.S
	@echo [$(CC)] $<
	@$(CC) $(CFLAGS) -c -MMD -o $@ $<

%.o: %.c
	@echo [$(CC)] $<
	@$(CC) $(CFLAGS) -c -MMD -o $@ $<

%.o: %.cpp
	@echo [$(CXX)] $<
	@$(CXX) $(CXXFLAGS) -c -MMD -o $@ $<
