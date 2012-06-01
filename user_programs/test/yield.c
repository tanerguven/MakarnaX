#ifdef _USER_PROGRAM
#include <user.h>
#endif

#include <stdio.h>
#include <string.h>

int y1();
int y2();

struct {
	const char *name;
	const char *description;
	int (*function)();
} tests[] = {
	{"y1", "y1", y1},
	{"y2", "y2 - tusa basilmasini bekleyen", y2},
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

int y1() {
	int i;

	for (i = 1 ; i < 6 ; i++) {
		printf("task %d, %d. kez calisiyor\n", getpid(), i);
		sys_yield();
	}
	printf("task %d bitti\n");
	return 0;

}

int y2() {
	int i;

	for (i = 1 ; i < 6 ; i++) {
		printf("task %d, %d. kez calisiyor\n", getpid(), i);
		printf("devam etmek icin bir tusa basin\n");
		getchar();
		sys_yield();
	}
	printf("task %d bitti\n");
	return 0;
}
