#ifndef _INC_SYSCALL_H
#define _INC_SYSCALL_H

#include <kernel/trap.h>
#include <types.h>

#define SYS_nosys		0
#define SYS_exit		1
#define SYS_fork		2
#define SYS_read		3
#define SYS_write		4
#define SYS_open		5
#define SYS_close		6
#define SYS_waitpid		7
#define SYS_creat		8
#define SYS_link		9
#define SYS_unlink		10
#define SYS_execve		11
#define SYS_chdir		12
#define SYS_time		13
#define SYS_mknod		14
#define SYS_chmod		15
#define SYS_lchown		16
#define SYS_break		17
#define SYS_oldstat		18
#define SYS_lseek		19
#define SYS_getpid		20
#define SYS_mount		21
#define SYS_umount		22
#define SYS_setuid		23
#define SYS_getuid		24
#define SYS_stime		25
#define SYS_ptrace		26
#define SYS_alarm		27
#define SYS_oldfstat	28
#define SYS_pause		29
#define SYS_utime		30
#define SYS_stty		31
#define SYS_gtty		32
#define SYS_access		33
#define SYS_nice		34
#define SYS_ftime		35
#define SYS_sync		36
#define SYS_kill		37
#define SYS_rename		38
#define SYS_mkdir		39
#define SYS_rmdir		40
#define SYS_dup			41
#define SYS_pipe		42
#define SYS_times		43
#define SYS_prof		44
#define SYS_brk			45
#define SYS_setgid		46
#define SYS_getgid		47
#define SYS_signal		48
#define SYS_geteuid		49
#define SYS_getegid		50
#define SYS_readdir		89
#define SYS_stat		106
#define SYS_lstat		107
#define SYS_fstat		108
#define SYS_ipc			117
#define SYS_getcwd		183
#define SYS_cputs		901
#define SYS_cgetc		902
#define SYS_yield		903
#define SYS_wait		904
#define SYS_dongu		905
#define SYS_sleep		906
#define MAX_SYSCALL_COUNT 1000


#define SYSCALL_DEFINE0(name) \
	asmlink void sys_##name() { \
	Trapframe* tf __attribute__((unused)) = task_curr->registers();	\

#define SYSCALL_DEFINE1(name, type1, var1) \
	SYSCALL_DEFINE0(name) \
	type1 var1 = (type1)get_param1(tf);

#define SYSCALL_DEFINE2(name, type1, var1, type2, var2) \
	SYSCALL_DEFINE1(name, type1, var1);					\
	type2 var2 = (type2)get_param2(tf);

#define SYSCALL_DEFINE3(name, type1, var1, type2, var2, type3, var3) \
	SYSCALL_DEFINE2(name, type1, var1, type2, var2); \
	type3 var3 = (type3)get_param3(tf);

#define SYSCALL_DEFINE4(name, type1, var1, type2, var2, type3, var3, type4, var4) \
	SYSCALL_DEFINE3(name, type1, var1, type2, var2, type3, var3); \
	type4 var4 = (type4)get_param4(tf);

#define SYSCALL_END(name) }

#define SYSCALL_RETURN(val) \
	set_return(task_curr->registers(), (uint32_t)val)	\


static inline int32_t syscall(int num, uint32_t a1, uint32_t a2, uint32_t a3, uint32_t a4, uint32_t a5)
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

	return ret;
}


#define _syscall0(r, name) \
	r name() {											\
		return (r)syscall(SYS_##name, 0, 0, 0, 0, 0);	\
	}

#define _syscall1(r, name, t1, p1) \
	r name(t1 p1) { \
		return (r)(syscall(SYS_##name, (uint32_t)p1, 0, 0, 0, 0));	\
	}

#define _syscall2(r, name, t1, p1, t2, p2)			\
	r name(t1 p1, t2 p2) {													\
		return (r)syscall(SYS_##name, (uint32_t)p1, (uint32_t)p2, 0, 0, 0); \
	}

#define _syscall3(r, name, t1, p1, t2, p2, t3, p3)							\
	r name(t1 p1, t2 p2, t3 p3) {												\
		return (r)syscall(SYS_##name, (uint32_t)p1, (uint32_t)p2, (uint32_t)p3, 0, 0); \
	}

#define _syscall4(r, name, t1, p1, t2, p2, t3, p3, t4, p4)					\
	r name(t1 p1, t2 p2, t3 p3, t4 p4) {										\
		return (r)syscall(SYS_##name, (uint32_t)p1, (uint32_t)p2, (uint32_t)p3, (uint32_t)p4, 0); \
	}

#define _syscall5(r, name, t1, p1, t2, p2, t3, p3, t4, p4, t5, p5)			\
	r name(t1 p1, t2 p2, t3 p3, t4 p4, t5 p5) {								\
		return (r)syscall(SYS_##name, (uint32_t)p1, (uint32_t)p2, (uint32_t)p3, (uint32_t)p4, (uint32_t)p5); \
	}

#endif /* _INC_SYSCALL_H */
