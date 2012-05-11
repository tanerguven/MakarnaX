D = ..
include ../Makefile.inc

OBJ_LIB = \
	genel_fonksiyonlar.o\
	vsprintf.o\

OBJ_USER_LIB = \
	syscall.o\
	stdio.o\
	lib.o\

DEPS_LIB = $(OBJ_LIB:.o=.d)
DEPS_USER_LIB = $(OBJ_USER_LIB:.o=.d)

all: user_lib

-include $(DEPS_LIB)
-include $(DEPS_USER_LIB)

libstring:
	@cd string; make download; make compile;

lib: $(OBJ_LIB)
	@echo [ld] lib.O
	@$(LD) -r -o lib.O $(OBJ_LIB)

# FIXME: --
user_lib: libstring lib $(OBJ_USER_LIB)
	@cp string/string.a user_lib.a
	@echo [ar] user_lib.a
	@ar r user_lib.a $(OBJ_USER_LIB) $(OBJ_LIB)

clean:
	cd string; make clean;
	rm -f *.o *.O *.a *.d;
