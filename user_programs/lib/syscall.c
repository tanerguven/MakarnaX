#include <types.h>
#include <kernel/syscall.h>
#include <kernel/trap.h>

#include "sys/syscall.h"

int getpid() {
	return syscall(SYS_getpid, 0, 0, 0, 0, 0, 0);
}

void _exit(int error_no) {
	syscall(SYS_exit, 1, error_no, 0, 0, 0, 0);
	while(1);
}

void sys_yield() {
	syscall(SYS_yield, 0, 0, 0, 0, 0, 0);
}

int fork() {
	return syscall(SYS_fork, 0, 0, 0, 0, 0, 0);
}

int wait(int *status) {
	return syscall(SYS_wait, 0, (uint32_t)status, 0, 0, 0, 0);
}

void sys_dongu() {
	syscall(SYS_dongu, 0, 0, 0, 0, 0, 0);
}

// FIXME: --
/* #include <user.h> */
/* sighandler_t signal(int signum, sighandler_t func) { */
/* 	return (sighandler_t)syscall(SYS_signal, 0, signum, (uint32_t)func, 0, 0, 0); */
/* } */

unsigned int alarm(unsigned int seconds) {
	return syscall(SYS_alarm, 0, seconds, 0, 0, 0, 0);
}

int kill(int pid, int sig) {
	return syscall(SYS_kill, 0, pid, sig, 0, 0, 0);
}

int pause() {
	return syscall(SYS_pause, 0, 0, 0, 0, 0, 0);
}

int sys_ipc(unsigned int ipc_no, int a1, int a2, int a3, int a4) {
	return syscall(SYS_ipc, 0, ipc_no, a1, a2, a3, a4);
}

int sleep(unsigned int seconds) {
	return syscall(SYS_sleep, 0, seconds, 0, 0, 0, 0);
}

int brk(void *addr) {
	return syscall(SYS_brk, 0, (uint32_t)addr, 0, 0, 0, 0);
}

void *sbrk(unsigned int increment) {
	int r;
	char *b;

	b = (char*)syscall(SYS_sbrk, 0, 0, 0, 0, 0, 0);
	r = brk(b+increment);
	if (r == 0)
		return b;
	return (void*)syscall(SYS_sbrk, 0, increment, 0, 0, 0, 0);
}

int open(const char *filename, int flags, int mode) {
	return (int)syscall(SYS_open, 0, (uint32_t)filename, (uint32_t)flags,
						(uint32_t)mode, 0, 0);
}

int close(unsigned int fd) {
	return (int)syscall(SYS_close, 0, (uint32_t)fd, 0, 0, 0, 0);
}

size_t read(unsigned int fd, char *buf, unsigned int count) {
	/* stdin yok, cgetc kullan */
	if (fd == 0) {
		int c;
		/* bir durumdan dolayi (alarm vb.) wake up olursa diye kontrol */
		while( (c = syscall(SYS_cgetc, 0, 0, 0, 0, 0, 0)) == 0);
		buf[0] = c;
		return 1;
	}
	return (size_t)syscall(SYS_read, 0, fd, (uint32_t)buf, count, 0, 0);
}

int chdir(const char *path) {
	return (int)syscall(SYS_chdir, 0, (uint32_t)path, 0, 0, 0, 0);
}

char *getcwd(char *buf, size_t size) {
	return (char*)syscall(SYS_getcwd, 0, (uint32_t)buf, size, 0, 0, 0);
}

int execve(const char *path, char *const argv[], char *const envp[]) {
	return (int)syscall(SYS_execve, 0, (uint32_t)path, (uint32_t)argv, 0, 0, 0);
}

void chown() {
	syscall(50, 0, 0, 0, 0, 0, 0);
}

void gettimeofday() {
	syscall(52, 0, 0, 0, 0, 0, 0);
}

void isatty() {
	syscall(53, 0, 0, 0, 0, 0, 0);
}

void link() {
	syscall(54, 0, 0, 0, 0, 0, 0);
}

void lseek() {
	syscall(55, 0, 0, 0, 0, 0, 0);
}

void readlink() {
	syscall(56, 0, 0, 0, 0, 0, 0);
}

void symlink() {
	syscall(57, 0, 0, 0, 0, 0, 0);
}

void unlink() {
	syscall(58, 0, 0, 0, 0, 0, 0);
}

size_t write(int fd, char *buf, int nbytes) {
	/* stdout tanimli degil, cputs kullan */
	if (fd == 1) {
		syscall(SYS_cputs, 0, (uint32_t)buf, nbytes, 0, 0, 0);
		return nbytes;
	}
	return syscall(58, 0, 0, 0, 0, 0, 0);
}
