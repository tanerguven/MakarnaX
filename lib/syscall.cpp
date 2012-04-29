#include <user.h>

#include <types.h>
#include <kernel/syscall.h>

asmlink _syscall0(int, getpid)
asmlink _syscall1(int, exit, int, error_no)
asmlink _syscall0(int, fork)
asmlink _syscall1(int, wait, int*, status)
asmlink _syscall2(sighandler_t, signal, int, signum, sighandler_t, func)
asmlink _syscall1(unsigned int, alarm, unsigned int, seconds)
asmlink _syscall2(int, kill, int, pid, int, sig)
asmlink _syscall0(int, pause)
asmlink _syscall5(int, ipc, unsigned int, ipc_no, int, a1, int, a2, int, a3, int, a4)
asmlink _syscall1(int, sleep, unsigned int, seconds)
asmlink _syscall3(int, open, const char*, filename, int, flags, int, mode)
asmlink _syscall1(int, close, unsigned int, fd)
asmlink _syscall3(size_t, read, unsigned int, fd, char*, buf, size_t, size)
asmlink _syscall3(size_t, write, unsigned int, fd, const char*, buf, size_t, size)
asmlink _syscall1(int, chdir, const char*, path)
asmlink _syscall2(char*, getcwd, char*, buf, size_t, size)
asmlink _syscall2(int, stat, const char*, path, struct stat*, buf)
asmlink _syscall2(int, execve, const char*, path, char* const*, argv)
asmlink _syscall2(int, mkdir, const char*, pathname, int, mode)
asmlink _syscall1(int, rmdir, const char*, pathname)
asmlink _syscall2(int, creat, const char*, pathname, int, mode)
asmlink _syscall1(int, unlink, const char*, pathname)

asmlink int brk(void *addr) {
	ptr_t new_brk = (ptr_t)syscall(SYS_brk, (uint32_t)addr, 0, 0, 0, 0);
	if (new_brk != addr)
		return -1;
	return 0;
}

asmlink void *sbrk(unsigned int increment) {
	ptr_t brk = (ptr_t)syscall(SYS_brk, 0, 0, 0, 0, 0);
	return (void*)syscall(SYS_brk, (uint32_t)(brk + increment), 0, 0, 0, 0);
}

asmlink void sys_cputs(const char *s, size_t len) {
	syscall(SYS_cputs, (uint32_t)s, len, 0, 0, 0);
}

asmlink int sys_cgetc() {
	int c;
	/* bir durumdan dolayi (alarm vb.) wake up olursa diye kontrol */
	while( (c = syscall(SYS_cgetc, 0, 0, 0, 0, 0)) == 0);
	return c;
}

asmlink void sys_yield() {
	syscall(SYS_yield, 0, 0, 0, 0, 0);
}

asmlink void sys_dongu() {
	syscall(SYS_dongu, 0, 0, 0, 0, 0);
}
