D = ..
include $(D)/Makefile.inc

kernel_dirs = \
	fs \
	memory \
	ipc \
INCLUDE_DIRS = $(kernel_dirs:=/Makefile.in)

SRC = $(wildcard *.S) $(wildcard *.c) $(wildcard *.cpp)

# kernel_dirs/Makefile.in dosyasindan SRC degiskenine dosyalari ekler
include $(INCLUDE_DIRS)

OBJS := $(SRC:.S=.o)
OBJS := $(OBJS:.c=.o)
OBJS := $(OBJS:.cpp=.o)
DEPS = $(OBJS:.o=.d)

all: $(OBJS)
	@echo [ld] kernel.O
	@$(LD) -r -o kernel.O $(OBJS)

-include $(DEPS)

clean:
	find . -name "*.o" | xargs rm -f
	find . -name "*.d" | xargs rm -f
	rm -f kernel.O
