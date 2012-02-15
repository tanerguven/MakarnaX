#include <user.h>

#include <types.h>
#include <stdio.h>
#include <kernel/trap.h>
#include <kernel/syscall.h>

#include <asm/x86.h>


static inline int32_t
syscall(int num, int check, uint32_t a1, uint32_t a2, uint32_t a3, uint32_t a4, uint32_t a5)
{
	int32_t ret;

	asm volatile("int %1\n"
		: "=a" (ret)
		: "i" (T_SYSCALL),
		  "a" (num),
		  "d" (a1),
		  "c" (a2),
		  "b" (a3),
		  "D" (a4),
		  "S" (a5)
		: "cc", "memory");

	if(check && ret > 0) {
		// FIXME: uyari verilmeli
		while(1);
	}

	return ret;
}

asmlink int getpid() {
	return syscall(SYS_getpid, 0, 0, 0, 0, 0, 0);
}

asmlink int exit(int error_no) {
	return syscall(SYS_exit, 1, error_no, 0, 0, 0, 0);
}

asmlink void sys_cputs(const char *s, size_t len) {
	syscall(SYS_cputs, 0, (uint32_t)s, len, 0, 0, 0);
}

asmlink int sys_cgetc() {
	int c;
	/* bir durumdan dolayi (alarm vb.) wake up olursa diye kontrol */
	while( (c = syscall(SYS_cgetc, 0, 0, 0, 0, 0, 0)) == 0);
	return c;
}

asmlink void sys_yield() {
	syscall(SYS_yield, 0, 0, 0, 0, 0, 0);
}

asmlink int fork() {
	return syscall(SYS_fork, 0, 0, 0, 0, 0, 0);
}

asmlink int wait(int *status) {
	return syscall(SYS_wait, 0, (uint32_t)status, 0, 0, 0, 0);
}

asmlink void sys_dongu() {
	syscall(SYS_dongu, 0, 0, 0, 0, 0, 0);
}

asmlink sighandler_t signal(int signum, sighandler_t func) {
	return (sighandler_t)syscall(SYS_signal, 0, signum, (uint32_t)func, 0, 0, 0);
}

asmlink unsigned int alarm(unsigned int seconds) {
	return syscall(SYS_alarm, 0, seconds, 0, 0, 0, 0);
}

asmlink int kill(int pid, int sig) {
	return syscall(SYS_kill, 0, pid, sig, 0, 0, 0);
}

asmlink int pause() {
	return syscall(SYS_pause, 0, 0, 0, 0, 0, 0);
}

asmlink int sys_ipc(unsigned int ipc_no, int a1, int a2, int a3, int a4) {
	return syscall(SYS_ipc, 0, ipc_no, a1, a2, a3, a4);
}

asmlink int sleep(unsigned int seconds) {
	return syscall(SYS_sleep, 0, seconds, 0, 0, 0, 0);
}

asmlink int brk(void *addr) {
	return syscall(SYS_brk, 0, (uint32_t)addr, 0, 0, 0, 0);
}

asmlink void *sbrk(unsigned int increment) {
	return (void*)syscall(SYS_sbrk, 0, increment, 0, 0, 0, 0);
}

asmlink int open(const char *filename, int flags, int mode) {
	return (int)syscall(SYS_open, 0, (uint32_t)filename, (uint32_t)flags,
				   (uint32_t)mode, 0, 0);
}

asmlink int close(unsigned int fd) {
	return (int)syscall(SYS_close, 0, (uint32_t)fd, 0, 0, 0, 0);
}

asmlink size_t read(unsigned int fd, char *buf, unsigned int count) {
	return syscall(SYS_read, 0, fd, (uint32_t)buf, count, 0, 0);
}

asmlink int readdir(unsigned int fd, struct dirent *dirent, unsigned int count) {
	return syscall(SYS_readdir, 0, fd, (uint32_t)dirent, count, 0, 0);
}
