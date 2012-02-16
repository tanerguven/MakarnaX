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

	r = close(r);
	printf("close r: %d\n", r);
	return 0;
}


int readfile() {
	int r;
	int fd1, fd2;
	char buf[1000];

	/* dosya1 isimli dosyayi ac */
	printf("open dosya1\n");
	fd1 = open("dosya1", 0, 0);
	if (fd1 < 0)
		return fd1;
	printf("fd1: %d\n", fd1);

	r = read(fd1, &buf, 10);
	printf("r %d\n", r);
	printf("buf: %s\n", buf);

	/* forktest programini dosya olarak ac */
	printf("\nopen forktest\n");
	fd2 = open("forktest", 0, 0);
	if (fd2 < 0)
		return fd2;
	printf("fd2: %d\n", fd2);

	r = read(fd2, &buf, 10);
	printf("r %d\n", r);
	printf("buf: %s\n", buf);

	printf("\nclose dosya1\n");
	r = close(fd1);
	if (r < 0) {
		printf("! close dosya1\n");
		return r;
	}

	printf("open dosya2\n");
	fd1 = open("dosya2", 0, 0);
	if (fd1 < 0)
		return fd1;
	printf("fd1: %d\n", fd1);

	r = read(fd1, &buf, 10);
	printf("r %d\n", r);
	printf("buf: %s\n", buf);

	close(fd1);
	close(fd2);

	return 0;
}
