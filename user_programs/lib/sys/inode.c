#include <kernel/syscall.h>

asmlink _syscall1(int, chdir, const char*, path)
asmlink _syscall2(int, getcwd, char*, buf, size_t, size)

void chown() {
	syscall(50, 0, 0, 0, 0, 0);
}

void link() {
	syscall(54, 0, 0, 0, 0, 0);
}

void readlink() {
	syscall(56, 0, 0, 0, 0, 0);
}

void symlink() {
	syscall(57, 0, 0, 0, 0, 0);
}

void unlink() {
	syscall(58, 0, 0, 0, 0, 0);
}
