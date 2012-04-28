#include <kernel/syscall.h>

// FIXME: --
/* asmlink _syscall2(sighandler_t, signal, int, signum, sighandler_t, func) */
asmlink _syscall1(unsigned int, alarm, unsigned int, seconds)
asmlink _syscall2(int, kill, int, pid, int, sig)
asmlink _syscall0(int, pause)
