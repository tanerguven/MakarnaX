#include <types.h>
#include <kernel/syscall.h>

asmlink _syscall0(int, getpid)
asmlink _syscall0(int, fork)
asmlink _syscall1(int, wait, int*, status)
// FIXME: --
/* asmlink _syscall2(sighandler_t, signal, int, signum, sighandler_t, func) */
asmlink _syscall1(unsigned int, alarm, unsigned int, seconds)
asmlink _syscall2(int, kill, int, pid, int, sig)
asmlink _syscall0(int, pause)
asmlink _syscall5(int, ipc, unsigned int, ipc_no, int, a1, int, a2, int, a3, int, a4)
asmlink _syscall1(int, sleep, unsigned int, seconds)

#define SYS__exit SYS_exit
#define return while(1)
asmlink _syscall1(void, _exit, int, error_no)
#undef return

void sys_dongu() {
	syscall(SYS_dongu, 0, 0, 0, 0, 0);
}

void sys_yield() {
	syscall(SYS_yield, 0, 0, 0, 0, 0);
}

#define SYS__brk SYS_brk
asmlink _syscall1(void*, _brk, void*, addr)


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

asmlink _syscall3(int, open, const char*, filename, int, flags, int, mode)
asmlink _syscall1(int, close, unsigned int, fd)
asmlink _syscall1(int, chdir, const char*, path)
asmlink _syscall2(int, getcwd, char*, buf, size_t, size)
asmlink _syscall3(int, execve, const char*, path, char *const*, argv, char *const*, envp)

void chown() {
	syscall(50, 0, 0, 0, 0, 0);
}

void gettimeofday() {
	syscall(52, 0, 0, 0, 0, 0);
}

void isatty() {
	syscall(53, 0, 0, 0, 0, 0);
}

void link() {
	syscall(54, 0, 0, 0, 0, 0);
}

void lseek() {
	syscall(55, 0, 0, 0, 0, 0);
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

size_t read(unsigned int fd, char *buf, unsigned int count) {
	/* stdin yok, cgetc kullan */
	if (fd == 0) {
		int c;
		/* bir durumdan dolayi (alarm vb.) wake up olursa diye kontrol */
		while( (c = syscall(SYS_cgetc, 0, 0, 0, 0, 0)) == 0);
		buf[0] = c;
		return 1;
	}
	return (size_t)syscall(SYS_read, fd, (uint32_t)buf, count, 0, 0);
}

size_t write(int fd, char *buf, int nbytes) {
	/* stdout tanimli degil, cputs kullan */
	if (fd == 1) {
		syscall(SYS_cputs, (uint32_t)buf, nbytes, 0, 0, 0);
		return nbytes;
	}
	return syscall(58, 0, 0, 0, 0, 0);
}
