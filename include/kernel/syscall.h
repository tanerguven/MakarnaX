#ifndef _INC_SYSCALL_H
#define _INC_SYSCALL_H

#define SYS_exit		1
#define SYS_fork		2
#define SYS_read		3
/* #define SYS_write 4 */
#define SYS_open		5
#define SYS_close		6
/* #define SYS_waitpid		  7 */
/* #define SYS_creat		  8 */
/* #define SYS_link		  9 */
/* #define SYS_unlink		 10 */
/* #define SYS_execve		 11 */
#define SYS_chdir		12
/* #define SYS_time		 13 */
/* #define SYS_mknod		 14 */
/* #define SYS_chmod		 15 */
/* #define SYS_lchown		 16 */
/* #define SYS_break		 17 */
/* #define SYS_oldstat		 18 */
/* #define SYS_lseek		 19 */
#define SYS_getpid		20
/* #define SYS_mount		 21 */
/* #define SYS_umount		 22 */
/* #define SYS_setuid		 23 */
/* #define SYS_getuid		 24 */
/* #define SYS_stime		 25 */
/* #define SYS_ptrace		 26 */
#define SYS_alarm		27
/* #define SYS_oldfstat		 28 */
#define SYS_pause		29
/* #define SYS_utime		 30 */
/* #define SYS_stty		 31 */
/* #define SYS_gtty		 32 */
/* #define SYS_access		 33 */
/* #define SYS_nice		 34 */
/* #define SYS_ftime		 35 */
/* #define SYS_sync		 36 */
#define SYS_kill		37
/* #define SYS_rename		 38 */
/* #define SYS_mkdir		 39 */
/* #define SYS_rmdir		 40 */
/* #define SYS_dup		 41 */
/* #define SYS_pipe		 42 */
/* #define SYS_times		 43 */
/* #define SYS_prof		 44 */
#define SYS_brk			45
/* #define SYS_setgid		 46 */
/* #define SYS_getgid		 47 */
#define SYS_signal		48
/* #define SYS_geteuid		 49 */
/* #define SYS_getegid		 50 */
#define SYS_readdir		89
#define SYS_stat		106
/* #define SYS_lstat		107 */
/* #define SYS_fstat		108 */
#define SYS_ipc			117
#define SYS_getcwd		183
#define SYS_cputs		1001
#define SYS_cgetc		1002
#define SYS_yield		1003
#define SYS_wait		1004
#define SYS_dongu		1005
#define SYS_sleep		1006
// FIXME: --
#define SYS_sbrk		1007

#endif /* _INC_SYSCALL_H */
