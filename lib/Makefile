D = ..
include ../Makefile.inc

OBJ_LIB = \
	genel_fonksiyonlar.o\
	vsprintf.o\
	readline.o\

OBJ_USER_LIB = \
	syscall.o\
	stdio.o\
	lib.o\

all: user_lib

libstring:
	cd string; make;

lib: $(OBJ_LIB)
	$(LD) -r -o lib.O $(OBJ_LIB)

# FIXME: --
user_lib: libstring lib $(OBJ_USER_LIB)
	cp string/string.a user_lib.a
	ar r user_lib.a $(OBJ_USER_LIB) $(OBJ_LIB)

clean:
	cd string; make clean;
	rm -f *.o *.O *.a;
