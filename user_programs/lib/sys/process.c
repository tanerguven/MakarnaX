#include <kernel/syscall.h>

#define SYS__exit SYS_exit

asmlink _syscall0(int, getpid)
asmlink _syscall0(int, fork)
asmlink _syscall3(int, execve, const char*, path, char *const*, argv, char *const*, envp)

#define return while(1)
asmlink _syscall1(void, _exit, int, error_no)
#undef return
