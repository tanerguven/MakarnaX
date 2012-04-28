#include <kernel/syscall.h>

asmlink _syscall5(int, ipc, unsigned int, ipc_no, int, a1, int, a2, int, a3, int, a4)

void sys_dongu() {
	syscall(SYS_dongu, 0, 0, 0, 0, 0);
}

void sys_yield() {
	syscall(SYS_yield, 0, 0, 0, 0, 0);
}

void gettimeofday() {
	syscall(52, 0, 0, 0, 0, 0);
}

void isatty() {
	syscall(53, 0, 0, 0, 0, 0);
}
