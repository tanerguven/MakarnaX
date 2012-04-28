#include <kernel/syscall.h>

#define SYS__brk SYS_brk
static _syscall1(void*, _brk, void*, addr)

int brk(void *addr) {
	ptr_t new_brk = (ptr_t)_brk(addr);
	if (new_brk != addr)
		return -1;
	return 0;
}

void *sbrk(unsigned int increment) {
	// TODO: curr_brk saklanirsa, iki sistem cagrisi yapmaya gerek kalmaz
	ptr_t old_brk = (ptr_t)_brk(0);
	ptr_t new_brk = (ptr_t)_brk(old_brk + increment);

	if (new_brk != old_brk + increment)
		return (void*)-1;

	return old_brk;
}
