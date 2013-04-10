#include <sys/stat.h>
#include <kernel/syscall.h>

asmlink _syscall2(int, stat, const char*, path, struct stat*, buf)
asmlink _syscall2(int, fstat, int, fd, struct stat*, buf)
asmlink _syscall2(int, lstat, const char*, path, struct stat*, buf)
