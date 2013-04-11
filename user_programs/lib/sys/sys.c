#include <kernel/syscall.h>

struct timeval;
struct timezone;

asmlink _syscall5(int, ipc, unsigned int, ipc_no, int, a1, int, a2, int, a3, int, a4)
asmlink _syscall2(int, gettimeofday, struct timeval*, tv, struct timezone*, tz)

void sys_dongu() {
	syscall(SYS_dongu, 0, 0, 0, 0, 0);
}

void sys_yield() {
	syscall(SYS_yield, 0, 0, 0, 0, 0);
}

void isatty() {
	syscall(53, 0, 0, 0, 0, 0);
}

int dup2(int oldfd, int newfd) {
	return syscall(54, 0, 0, 0, 0, 0);
}

int pipe(int pipefd[2]) {
	return syscall(55, 0, 0, 0, 0, 0);
}

int fcntl(int fd, int cmd, ...) {
	return syscall(4000 + fd, 0, 0, 0, 0, 0);
}

int sigaction(int signum, void *act, void *oldact) {
	return syscall(57, 0, 0, 0, 0, 0);
}

int getppid() {
	return syscall(58, 0, 0, 0, 0, 0);
}

int geteuid() {
	return 1000;
	/* return syscall(2000 + 59, 0, 0, 0, 0, 0); */
}

int getgid() {
	return 1000;
	/* return syscall(2000 + 60, 0, 0, 0, 0, 0); */
}

int getegid() {
	return 1000;
	/* return syscall(2000 + 61, 0, 0, 0, 0, 0); */
}

int getgroups(int size, int list[]) {
	return syscall(62, 0, 0, 0, 0, 0);
}

int getuid() {
	return 1000;
	/* return syscall(2000 + 64, 0, 0, 0, 0, 0); */
}
int sigprocmask(int how, void *set, void *oldset) {
	return syscall(65, 0, 0, 0, 0, 0);
}
void *opendir(const char *name) {
	return (void*)syscall(66, 0, 0, 0, 0, 0);
}
void *readdir(void *dirp) {
	return (void*)syscall(67, 0, 0, 0, 0, 0);
}
int closedir(void *dirp) {
	return syscall(68, 0, 0, 0, 0, 0);
}
int sigsuspend(void *mask) {
	return syscall(69, 0, 0, 0, 0, 0);
}
int wait3(int *status, int options, void *rusage) {
	return syscall(70, 0, 0, 0, 0, 0);
}
int setpgid(int pid) {
	return syscall(72, 0, 0, 0, 0, 0);
}
int umask(int mask) {
	return syscall(73, 0, 0, 0, 0, 0);
}
int getpgrp() {
	return syscall(74, 0, 0, 0, 0, 0);
}
int tcsetpgrp(int fd, int pgrp) {
	return syscall(75, 0, 0, 0, 0, 0);
}
int tcgetpgrp(int fd) {
	return syscall(76, 0, 0, 0, 0, 0);
}
