D = ../..
include $(D)/Makefile.in
include ../MakarnaXMakefile

CFLAGS = -nostdinc -O0 -g3 -Wall -Werror=implicit-function-declaration \
	${INCLUDES}

USER_LIBS = -lc -lmakarnax
LDFLAGS = -static -L$(R)/lib -L../lib

PROGRAMS = \
	hello_newlib.bin \

all: $(PROGRAMS)

ln_error-%:
	ln -sf ../test/cross_compile_error.bin ../test/$*;

ln_program-%:
	ln -sf ${CURDIR}/$* ../test/$*;

hello_newlib.bin: ln_error-"hello_newlib.bin" hello_newlib.o
	@echo [$(LD)] hello_newlib.bin
	@$(LD) $(LDFLAGS) -o hello_newlib.bin hello_newlib.o $(USER_LIBS)
	make ln_program-"hello_newlib.bin"

clean:
	rm -f *.o *.bin *.d