#include <sys/stat.h>

#include <types.h>

#include "syscall.h"
#include <kernel/syscall.h>

int stat(const char *path, struct stat *buf) {
	return (int)syscall(SYS_stat, 0, (uint32_t)path, (uint32_t)buf, 0, 0, 0);
}

int fstat(int fd, struct stat *buf) {
	return syscall(51, 0, 0, 0, 0, 0, 0);
}
