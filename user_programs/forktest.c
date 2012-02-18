#ifdef _USER_PROGRAM
#include <user.h>
#else
#include <unistd.h>
#include <stdlib.h> // exit
#endif

#include <stdio.h>
#include <sys/wait.h>
#include <signal.h>
#include <string.h>

int test1();
int test2();

struct {
	const char *name;
	const char *description;
	int (*function)();
} tests[] = {
	{"test1", "test1 - basit fork", test1},
	{"test2", "test2 - fork - wait", test2},
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

int test1() {
	int pid;
	int i = 10;
	int status;

	pid = fork();

	if (pid < 0) {
		printf("[%d] fork yapilamadi\n", getpid());
		return -1;
	}

	if (pid > 0) {
		printf("parent process calisiyor\n");
		printf("child id: %d\n", pid);
		wait(&status);

	} else if (pid == 0) {
		printf("child process calisiyor\n");
		sleep(1);
	}

	printf("%d\n", ++i);
	exit(0);
}

/**********************************************
 * test 2
 **********************************************/

int test2() {
	int pid;
	int status, w;

	pid = fork();

	if (pid < 0) {
		printf("fork yapilamadi\n");
		return -1;
	}

	if (pid > 0) {
		printf("parent process calisiyor\n");
		printf("child id: %d\n", pid);
		do {
			w = wait(&status);
			printf("pid %d: ", w);

			if (WIFEXITED(status)) {
				printf("exited, status=%d\n", WEXITSTATUS(status));
			} else if (WIFSIGNALED(status)) {
				printf("killed by signal %d\n", WTERMSIG(status));
			} else if (WIFSTOPPED(status)) {
				printf("stopped by signal %d\n", WSTOPSIG(status));
			}/*  else if (WIFCONTINUED(status)) { */
			/* printf("continued\n"); */
			/* } */

		} while (!WIFEXITED(status) && !WIFSIGNALED(status));

	} else if (pid == 0) {
		printf("child process %d calisiyor\n", getpid());
		sleep(1);
		/* sys_exit(1); */
		/* sys_kill(sys_getpid(), SIGSTOP); */
		kill(getpid(), SIGABRT);
		while(1);
	}

	printf("%d, sonlaniyor\n", getpid());

	return 0;
}
