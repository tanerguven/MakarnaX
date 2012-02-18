#ifdef _USER_PROGRAM
#include <user.h>
#define shmflg_server 1
#define shmflg_client 0

#else

#include <stdlib.h> // exit
#include <unistd.h>
#include <sys/shm.h>
#include <semaphore.h>
#define shmflg_server (IPC_CREAT | 0666)
#define shmflg_client (IPC_CREAT)

#endif

#include <stdio.h>
#include <string.h>

int shmtest();
int shmserver();
int shmclient();
int semtest();
int shmserver2();
int shmclient2();
int shmserver3();
int shmclient3();
int shmfork();

struct {
	const char *name;
	const char *description;
	int (*function)();
} tests[] = {
	{"shmtest", "shared memory", shmtest},
	{"shmserver", "shared memory server", shmserver},
	{"shmclient", "shared memory client", shmclient},
	{"semtest", "semaphore", semtest},
	{"shmserver2", "shmserver2", shmserver2},
	{"shmclient2", "shmclient2", shmclient2},
	{"shmserver3", "semaphore - shm server", shmserver3},
	{"shmclient3", "semaphore - shm client", shmclient3},
	{"shmfork", "shmfork", shmfork},
};
#define TEST_COUNT (sizeof(tests)/sizeof(tests[0]))

int main(int argc, char** argv) {
	int i;

	if (argc > 1) {
		for (i = 0 ; i < TEST_COUNT ; i++) {
			if (strcmp(argv[1], tests[i].name) == 0) {
				printf("%s:\n\n", tests[i].description);
				return tests[i].function();
			}
		}
	}

	printf("%s [test]\n", argv[0]);
	printf("tests:\n");
	for (i = 0 ; i < TEST_COUNT ; i++) {
		printf("  %s - %s\n", tests[i].name, tests[i].description);
	}
	return -1;
}

int shmtest() {
	int shmid;
	char *shm;

	shmid = shmget(5000, 100, shmflg_server);
	if (shmid < 0) {
		printf("shmget error %d\n", shmid);
		return -1;
	}
	printf("shmid: %d\n", shmid);

	shm = shmat(shmid, NULL, 0);
	if (shm == (char*)-1) {
		printf("shmat error\n");
		return -1;
	}
	printf("shm addr: %p\n", shm);

	strcpy(shm, "deneme");
	printf("write shared memory OK\n");

	if ( shmdt(shm) < 0 ) {
		printf("shmdt error\n");
		return -1;
	}
	printf("shmdt OK\n");

	/* burada segmantation fault olmali */
	strcpy(shm, "test");

	printf("HATA: bu satir calismamali\n");
	return -1;
}

int shmserver() {
	int shmid;
	char *shm;
	int pid;

	pid = getpid();
	printf("server pid: %d\n", pid);

	shmid = shmget(3999, 100, shmflg_server);
	if (shmid < 0) {
		printf("[%d] shmget error %d\n", pid, shmid);
		return -1;
	}
	printf("[%d] shmid: %d\n", pid, shmid);

	shm = shmat(shmid, NULL, 0);
	if (shm == (char*)-1) {
		printf("[%d] shmat error\n", pid);
		return -1;
	}
	printf("[%d] shm addr: %p\n", pid, shm);

	strcpy(shm, "hello");

	printf("[%d] write shared memory OK\n", pid);

	while (shm[6] == '\0');

	/* clientta islem yarida kesilebilir biraz daha zaman ver */
	sleep(1);

	printf("[%d] shared memory changed\n", pid);
	printf("[%d] printing shared memory\n", pid);
	printf("shared memory: %s\n", shm);

	return 0;
}

int shmclient() {
	int shmid;
	char *shm;
	int pid;
	char buf[100];

	pid = getpid();
	printf("client pid: %d\n", pid);

	shmid = shmget(3999, 100, shmflg_client);
	if (shmid < 0) {
		printf("[%d] shmget error\n", pid);
		return -1;
	}
	printf("[%d] shmid: %d\n", pid, shmid);

	shm = shmat(shmid, NULL, 0);
	if (shm == (char*)-1) {
		printf("[%d] shmat error\n", pid);
		return -1;
	}
	printf("[%d] shm addr: %p\n", pid, shm);

	printf("[%d] reading shared memory\n", pid);
	strcpy(buf, shm);
	printf("data: %s\n", buf);

	printf("[%d] directly printing shared memory\n", pid);
	printf("shared memory: %s\n", buf);

	printf("[%d] changing shared memory\n", pid);
	strcat(shm, " world!");
	printf("[%d] OK\n", pid);

	return 0;
}


sem_t sem;

int semtest() {
	int r;
	int sval;

	r = sem_init(&sem, 0, 1);
	printf("sem_init: %d\n", r);
	if (r < 0)
		exit(1);

	r = sem_getvalue(&sem, &sval);
	printf("sem_getvalue r:%d val:%d\n", r, sval);

	r = sem_wait(&sem);
	printf("sem_wait %d\n", r);

	r = sem_getvalue(&sem, &sval);
	printf("sem_getvalue r:%d val:%d\n", r, sval);

	r = sem_trywait(&sem);
	printf("sem_trywait %d\n", r);

	r = sem_getvalue(&sem, &sval);
	printf("sem_getvalue r:%d val:%d\n", r, sval);

	r = sem_post(&sem);
	printf("sem_post %d\n", r);

	r = sem_getvalue(&sem, &sval);
	printf("sem_getvalue r:%d val:%d\n", r, sval);

	return 0;
}

static void* open_shm(int key, size_t size, int flag) {
	void *shm;
	int shmid;

	shmid = shmget(key, size, flag);
	if (shmid < 0) {
		printf("[%d] shmget error (%d)\n", getpid(), key);
		exit(1);
	}

	shm = shmat(shmid, NULL, 0);
	if (shm == (char*)-1) {
		printf("[%d] shmat error (%d)\n", getpid(), shmid);
		exit(1);
	}
	return shm;
}

int shmserver2() {
	int i;

	char *shm;

	printf("server pid: %d\n", getpid());

	shm = (char*)open_shm(2999, 100, shmflg_server);

	shm[0] = 'a';
	shm[1] = 'b';

	sleep(1);
	for (i = 1 ; i < 0xFFFFFF ; i++) {
		char tmp;
		tmp = shm[1];
		shm[1] = shm[0];
		shm[0] = tmp;
		if (i % 0x200000 == 0)
			printf("[%d] %d\n", getpid(), i);
	}
	printf("server: shm[0]:%c shm[1]:%c\n", shm[0], shm[1]);
	sleep(1);
	return 0;
}

int shmclient2() {
	int i;
	char *shm;

	printf("client pid : %d\n", getpid());

	shm = (char*)open_shm(2999, 100, shmflg_client);

	sleep(1);
	for (i = 1 ; i < 0xFFFFFF ; i++) {
		char tmp;
		tmp = shm[1];
		shm[1] = shm[0];
		shm[0] = tmp;
		if (i % 0x200000 == 0)
			printf("[%d] %d\n", getpid(), i);
	}
	printf("client: shm[0]:%c shm[1]:%c\n", shm[0], shm[1]);
	sleep(1);
	return 0;
}




int shmserver3() {
	int i, r;
	char *shm;
	sem_t *sem;

	printf("server pid: %d\n", getpid());

	shm = (char*)open_shm(2999, 100, shmflg_server);
	sem = (sem_t*)open_shm(2998, sizeof(sem_t), shmflg_server);

	r = sem_init(sem, 1, 1);
	if (r < 0) {
		printf("sem init error\n");
		return -1;
	}

	shm[0] = 'a';
	shm[1] = 'b';

	sleep(1);
	printf("basladi\n");
	for (i = 1 ; i < 16000000 ; i++) {
		char tmp;
		sem_wait(sem);
		tmp = shm[1];
		shm[1] = shm[0];
		shm[0] = tmp;
		if (shm[0] == shm[1]) {
			printf("hata!!!\n");
			return 1;
		}
		sem_post(sem);
		if (i % 1600000 == 0)
			printf("[%d] %d\n", getpid(), i);
	}
	printf("bitti\n");
	sem_wait(sem);
	printf("server: shm[0]:%c shm[1]:%c\n", shm[0], shm[1]);
	sem_post(sem);

	sleep(1);

	return 0;
}


int shmclient3() {
	int i;
	char *shm;
	sem_t *sem;

	printf("client pid : %d\n", getpid());

	shm = (char*)open_shm(2999, 100, shmflg_client);
	sem = (sem_t*)open_shm(2998, sizeof(sem_t), shmflg_client);

	sleep(1);
	printf("basladi\n");
	for (i = 1 ; i < 16000000 ; i++) {
		char tmp;
		sem_wait(sem);
		tmp = shm[1];
		shm[1] = shm[0];
		shm[0] = tmp;
		if (shm[0] == shm[1]) {
			printf("hata!!!\n");
			return 1;
		}
		sem_post(sem);
		if (i % 1600000 == 0)
			printf("[%d] %d\n", getpid(), i);
	}
	printf("bitti\n");
	sem_wait(sem);
	printf("client shm[0]:%c shm[1]:%c\n", shm[0], shm[1]);
	sem_post(sem);

	sleep(1);

	return 0;
}

int shmfork() {
	char *shm;
	int pid;

	shm = (char*)open_shm(2999, 100, shmflg_server);

	pid = fork();
	if (pid < 0) {
		printf("fork error\n");
		return -1;
	}

	if (pid > 0) {
		printf("parent: %d\n", getpid());
		shm[0] = 'a';
		sleep(2);
	} else {
		sleep(1);
		printf("child: %d\n", getpid());
		if (shm[0] != 'a') {
			printf("!!! HATA\n");
		}
		shm[1] = 'b';
		printf("child shared memory OK\n");
	}

	shmdt(shm);
	printf("[%d] bitti\n", getpid());

	return 0;
}
