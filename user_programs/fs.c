#ifdef _USER_PROGRAM
#include <user.h>
#endif

#include <stdio.h>

int openfile();
int readfile();

struct {
	const char *name;
	const char *description;
	int (*function)();
} tests[] = {
	{"openfile", "openfile", openfile},
	{"readfile", "readfile", readfile},
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

int openfile() {
	int r;

	r = open("dosya1", 0, 0);
	printf("r %d\n", r);
	return 0;
}


int readfile() {
	int r;

	r = open("dosya1", 0, 0);
	if (r < 0)
		return r;

	char buf[1000];
	r = read(0, &buf, 10);

	printf("r %d\n", r);
	printf("buf: %s\n", buf);

	return 0;
}
