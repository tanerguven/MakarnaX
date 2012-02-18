#ifndef INC_LIB_H_
#define INC_LIB_H_

#include <types.h>

typedef void (*sighandler_t)(int);

#ifdef __cplusplus
extern "C" {
#endif

extern int getpid();
extern int exit(int error_code);
extern int fork();
extern sighandler_t signal(int signum, sighandler_t handler);
extern unsigned int alarm(unsigned int seconds);
extern int kill(int pid, int sig);
extern int pause();
extern int wait(int *status);
/* shared memory ve semaphore bu cagri uzerinden kullaniliyor */
extern int sys_ipc(unsigned int ipc_no, int a2, int a3, int a4, int a5);
extern int sleep(unsigned int seconds);
extern int brk(void *addr);
extern void *sbrk(unsigned int increment);

extern void sys_cputs(const char *s, size_t len);
extern int sys_cgetc();
extern void sys_yield();
extern void sys_dongu();

/******************************
 * File System
 ******************************/
struct stat;
extern int open(const char *filename, int flags, int mode);
extern int close(unsigned int fd);
extern size_t read(unsigned int fd, char *buf, unsigned int count);
extern int chdir(const char *path);
extern char *getcwd(char *buf, size_t size);
extern int stat(const char *path, struct stat *buf);

/******************************
 * Shared Memory
 ******************************/
struct shmid_ds {};
extern key_t shmget(key_t key, size_t size, int shmflg);
extern void *shmat(int shmid, const void *shmaddr, int shmflg);
extern int shmdt(const void *shmaddr);
extern int shmctl(int shmid, int cmd, struct shmid_ds *buf);

/***************************
 * semaphore
 ****************************/
typedef union {
	char __semaphore_info[32];
} sem_t;

extern int sem_init(sem_t *sem, int pshared, unsigned int value);
extern int sem_destroy(sem_t *sem);
extern int sem_wait(sem_t *sem);
extern int sem_trywait(sem_t *sem);
extern int sem_post(sem_t *sem);
extern int sem_getvalue(sem_t *sem, int *val);

#ifdef __cplusplus
}
#endif

#endif /* LIB_H_ */
