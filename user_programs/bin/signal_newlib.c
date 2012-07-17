#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <signal.h>
#include <unistd.h>

int test1();
int test2();
int test7();

struct {
	const char *name;
	const char *description;
	int (*function)();
} tests[] = {
	{"test1", "test1 - alarm testi", test1},
	{"test2", "test2 - 3 signal testi", test2},
	/* {"test3", "test3 - alarm + keyboard testi", test3}, */
	/* {"test4", "test4 - pause + signal", test4}, */
	/* {"test5", "test5 - sleep + signal", test5}, */
	/* {"test6", "test6 - pause + 3 signal", test6}, */
	{"test7", "test7", test7},
};
#define TEST_COUNT (sizeof(tests)/sizeof(tests[0]))

int main(int argc, char** argv) {
	int i;

	if (argc > 1) {
		for (i = 0 ; i < TEST_COUNT ; i++) {
			if (strcmp(argv[1], tests[i].name) == 0) {
				printf("%s: %s:\n\n", argv[0], tests[i].description);
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

/**********************************************
 * test 1
 **********************************************/

static int i_test1 = 0;

void catch(int sig) {
	i_test1++;
	printf("%d. catch_sig %d\n", i_test1, sig);
	signal(sig, catch);
	alarm(1);
	if (i_test1 == 10)
		exit(0);
}

int test1() {
	int r;

	r = (int)signal(SIGALRM, catch);
	printf("sys_signal return: %08x\n", r);

	r = alarm(1);
	printf("sys_alarm return: %d\n", r);

	while(1);

	return 0;
}


/**********************************************
 * test 2
 **********************************************/

void sigquit() {
	printf("CHILD: SIGQUIT received\n");
	printf("exiting\n");
	exit(0);
}

void sigint() {
	printf("CHILD: SIGINT received\n");
}

void test2_handler(int sig) {
	printf("CHILD: signal %d received\n", sig);
}

int test2() {
	int pid;
	int r;

	if ((pid = fork()) < 0) {
		printf("fork error\n");
		return -2;
	}

	if (pid == 0) {
		/* child */
		printf("child\n");
		r = (int)signal(SIGINT, sigint);
		printf("sys_signal return = %08x\n", r);
		r = (int)signal(SIGQUIT, sigquit);
		printf("sys_signal return = %08x\n", r);
		r = (int)signal(SIGHUP, test2_handler);

		while(1);

	} else {
		sleep(1);

		/* parent */
		printf("PARENT: sendign SIGINT\n");
		r = kill(pid, SIGINT);
		if (r < 0)
			printf("sys_kill err: %d\n", r);

		printf("PARENT: sendign SIGQUIT\n");
		r = kill(pid, SIGQUIT);
		if (r < 0)
			printf("sys_kill err: %d\n", r);

		printf("PARENT: sendign SIGUP\n");
		r = kill(pid, SIGHUP);
		if (r < 0)
			printf("sys_kill err: %d\n", r);

		sleep(2);
	}

	return 0;
}

/**********************************************
 * test 7
 **********************************************/

void test7_h1(int sig) {
	int r;
	printf("test7_h1 basladi\n");
	r = sleep(100);
	printf("test7_h1 bitti r:%d\n", r);
}

void test7_h2(int sig) {
	int r;
	printf("test7_h2\n");
	r = sleep(10);
	printf("test7_h2 bitti r:%d\n", r);
}

inline void send_sig(int sig, int pid) {
	int r;
	printf("PARENT: sendign %d\n", sig);
	r = kill(pid, sig);
	if (r < 0)
		printf("sys_kill err: %d\n", r);
}

int test7() {
	int pid;
	int r;

	if ((pid = fork()) < 0) {
		printf("fork error\n");
		return -2;
	}

	if (pid == 0) {
		printf("child\n");
		signal(SIGHUP, test7_h1);
		signal(SIGINT, test7_h1);
		signal(SIGQUIT, test7_h2);
		signal(SIGUSR1, test7_h2);

		r = pause();
		printf("pause r: %d\n", r);

		while (1);

	} else {
		sleep(1);
		send_sig(SIGHUP, pid);
		sleep(1);
		send_sig(SIGHUP, pid);
		sleep(1);
		send_sig(SIGHUP, pid);
		sleep(1);
		send_sig(SIGINT, pid);
		sleep(1);
		send_sig(SIGQUIT, pid);
		sleep(1);
		send_sig(SIGUSR1, pid);
		sleep(100);
	}

	return 0;
}
