#include <kernel/syscall.h>

asmlink _syscall1(int, chdir, const char*, path)
asmlink _syscall2(int, getcwd, char*, buf, size_t, size)

// FIXME: --
/* asmlink _syscall3(int, chown, const char *, path, int, owner, int, group) */

void chown() {
	syscall(2000 + 50, 0, 0, 0, 0, 0);
}

void link() {
	syscall(2000 + 54, 0, 0, 0, 0, 0);
}

void readlink() {
	syscall(2000 + 56, 0, 0, 0, 0, 0);
}

void symlink() {
	syscall(2000 + 57, 0, 0, 0, 0, 0);
}

void unlink() {
	syscall(2000 + 58, 0, 0, 0, 0, 0);
}
