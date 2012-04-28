#include "user.h"

/******************************
 * ipc
 ******************************/

#define SEMGET 1
#define SEMCTL 2
#define SEMOP 3
#define SEMDT 4
#define SEMWAIT 5
#define SEMPOST 6
#define SEMGETVAL 7
#define SHMGET 10
#define SHMAT 11
#define SHMDT 12
#define SHMCTL 12

asmlink key_t shmget(key_t key, size_t size, int shmflg) {
	return (key_t)ipc(SHMGET, key, size, shmflg, 0);
}

asmlink void *shmat(int shmid, const void *shmaddr, int shmflg) {
	return (void*)ipc(SHMAT, shmid, (uint32_t)shmaddr, shmflg, 0);
}

asmlink int shmdt(const void *shmaddr) {
	return ipc(SHMDT, (uint32_t)shmaddr, 0, 0, 0);
}

asmlink int shmctl(int shmid, int cmd, struct shmid_ds *buf) {
	return ipc(SHMCTL, shmid, cmd, (uint32_t)buf, 0);
}

asmlink int sem_init(sem_t *sem, int pshared, unsigned int value) {
	return ipc(SEMGET, (uint32_t)sem, pshared, value, 0);
}

asmlink int sem_destroy(sem_t *sem) {
	return ipc(SEMDT, (uint32_t)sem, 0, 0, 0);
}

asmlink int sem_wait(sem_t *sem) {
	return ipc(SEMWAIT, (uint32_t)sem, 0, 0, 0);
}

asmlink int sem_trywait(sem_t *sem) {
	return ipc(SEMOP, (uint32_t)sem, 0, 0, 0);
}

asmlink int sem_post(sem_t *sem) {
	return ipc(SEMPOST, (uint32_t)sem, 0, 0, 0);
}

int sem_getvalue(sem_t *sem, int *val) {
	return ipc(SEMGETVAL, (uint32_t)sem, 0, 0, 0);
}


/***************************
 * dirent
 ***************************/

#include <dirent.h>

asmlink DIR* opendir(const char *path) {
	int r;

	r = open(path, 0, 0);
	if (r < 0)
		return NULL;

	return NULL;
}

asmlink int closedir(DIR *dirp) {
	int r;

	r = close(dirp->fd);
	if (r < 0)
		return r;

	return 0;
}

asmlink struct dirent* readdir(DIR *dirp) {
	return NULL;
}
