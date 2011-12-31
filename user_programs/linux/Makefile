CC = gcc
CFLAGS = -O0 -g3 -Wall -c
CXX = $(CC)
CXXFLAGS = $(CFLAGS)

PROGRAMS = \
	hello.bin \
	forktest.bin \
	dongu.bin \
	signaltest.bin \
	ipctest.bin \
	divide_error.bin \

all: clean $(PROGRAMS)
	rm -rf ../*.o

hello.bin: ../hello.o
	gcc -o hello.bin ../hello.o $(LIBS)

forktest.bin: ../forktest.o
	gcc -o forktest.bin ../forktest.o $(LIBS)

dongu.bin: ../dongu.o
	gcc -o dongu.bin ../dongu.o $(LIBS)

signaltest.bin: ../signaltest.o
	gcc -o signaltest.bin ../signaltest.o $(LIBS)

ipctest.bin: ../ipctest.o
	gcc -o ipctest.bin ../ipctest.o $(LIBS) -lpthread

divide_error.bin: ../divide_error.o
	gcc -o divide_error.bin ../divide_error.o $(LIBS)

clean:
	rm -f ../*.o *.bin
