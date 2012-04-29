#include <kernel/syscall.h>
#include <types.h>

asmlink void sys_exit();
asmlink void sys_fork();
asmlink void sys_read();
asmlink void sys_write();
asmlink void sys_open();
asmlink void sys_close();
asmlink void sys_execve();
asmlink void sys_chdir();
asmlink void sys_getpid();
asmlink void sys_alarm();
asmlink void sys_pause();
asmlink void sys_kill();
asmlink void sys_brk();
asmlink void sys_signal();
asmlink void sys_readdir();
asmlink void sys_stat();
asmlink void sys_ipc();
asmlink void sys_getcwd();
asmlink void sys_mkdir();
asmlink void sys_rmdir();
asmlink void sys_creat();
asmlink void sys_unlink();

asmlink void sys_cputs();
asmlink void sys_cgetc();
asmlink void sys_yield();
asmlink void sys_wait();
asmlink void sys_dongu();
asmlink void sys_sleep();
asmlink void sys_nosys();

void (* const syscalls[])() = {
	[0 ... MAX_SYSCALL_COUNT] = sys_nosys,

	[SYS_exit] = sys_exit,
	[SYS_fork] = sys_fork,
	[SYS_read] = sys_read,
	[SYS_write] = sys_write,
	[SYS_open] = sys_open,
	[SYS_close] = sys_close,
	[SYS_execve] = sys_execve,
	[SYS_chdir] = sys_chdir,
	[SYS_getpid] = sys_getpid,
	[SYS_alarm] = sys_alarm,
	[SYS_pause] = sys_pause,
	[SYS_kill] = sys_kill,
	[SYS_brk] = sys_brk,
	[SYS_signal] = sys_signal,
	[SYS_readdir] = sys_readdir,
	[SYS_stat] = sys_stat,
	[SYS_ipc] = sys_ipc,
	[SYS_getcwd] = sys_getcwd,
	[SYS_mkdir] = sys_mkdir,
	[SYS_rmdir] = sys_rmdir,
	[SYS_creat] = sys_creat,
	[SYS_unlink] = sys_unlink,

	[SYS_cputs] = sys_cputs,
	[SYS_cgetc] = sys_cgetc,
	[SYS_yield] = sys_yield,
	[SYS_wait] = sys_wait,
	[SYS_dongu] = sys_dongu,
	[SYS_sleep] = sys_sleep,
};
