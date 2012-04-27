D = ../..
include $(D)/Makefile.inc
include ../MakarnaXMakefile

CFLAGS = -nostdinc -O0 -g3 -Wall -Werror=implicit-function-declaration \
	${INCLUDES}

USER_LIBS = -lc -lmakarnax
LDFLAGS = -static -L$(R)/lib -L../lib

PROGRAMS = \
	hello_newlib.bin \

all: errors $(PROGRAMS)

errors:
	ln -sf ../test/cross_compile_error.bin ../test/hello_newlib.bin

hello_newlib.bin: hello_newlib.o
	@echo [$(LD)] hello_newlib.bin
	@$(LD) $(LDFLAGS) -o hello_newlib.bin hello_newlib.o $(USER_LIBS)
	rm -f ../test/hello_newlib.bin
	ln -sf `pwd`/hello_newlib.bin ../test/hello_newlib.bin

clean:
	rm -f *.o *.bin *.d