D = ../..
include $(D)/Makefile.in
include ../MakarnaXMakefile

# CFLAGS = -nostdinc -O0 -g3 -Wall -Werror=implicit-function-declaration \
# 	${INCLUDES}

CFLAGS = -nostdinc -O0 -g3 -Wall ${INCLUDES}


PROGRAMS = \
	bin/hello_newlib \
	bin/signal_newlib

all: $(PROGRAMS)

.SECONDARY:

bin/%: %.o
	@echo [$(GCC)] $*
	@$(GCC) -o $(INITRD_BIN_FOLDER)/$* $*.o

clean:
	rm -f *.o *.bin *.d