#ifdef _USER_PROGRAM
#include <user.h>
# else
#include <unistd.h>
#include <fcntl.h> // open
#endif

#include <stdio.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>

int openfile();
int readfile();
int test_readdir();
int test_chdir();
int test_stat();

struct {
	const char *name;
	const char *description;
	int (*function)();
} tests[] = {
	{"openfile", "openfile", openfile},
	{"readfile", "readfile", readfile},
	{"readdir", "readdir", test_readdir},
	{"chdir", "chdir", test_chdir},
	{"stat", "stat", test_stat},
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

	r = read(fd1, buf, 10);
	printf("r %d\n", r);
	printf("buf: %s\n", buf);

	/* forktest programini dosya olarak ac */
	printf("\nopen forktest\n");
	fd2 = open("forktest", 0, 0);
	if (fd2 < 0)
		return fd2;
	printf("fd2: %d\n", fd2);

	r = read(fd2, buf, 10);
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

	r = read(fd1, buf, 10);
	printf("r %d\n", r);
	printf("buf: %s\n", buf);

	close(fd1);
	close(fd2);

	return 0;
}

int test_readdir() {
	DIR *dir;;
	struct dirent *dirent;

	dir = opendir(".");

	while((dirent = readdir(dir))) {
		printf("%s\n",dirent->d_name);
	}

	closedir(dir);
	return 0;
}

int test_chdir() {
	int r;
	char buf[256];

	getcwd(buf, 256);
	printf("cwd %s\n\n", buf);

	printf("chdir dir1\n");
	r = chdir("dir1");
	if (r < 0) {
		printf("r %d\n", r);
		return -1;
	}
	getcwd(buf, 256);
	printf("cwd %s\n\n", buf);

	printf("chdir dir11\n");
	r = chdir("dir11");
	if (r < 0) {
		printf("r %d\n", r);
		return -1;
	}
	getcwd(buf, 256);
	printf("cwd %s\n\n", buf);

	printf("chdir .\n");
	r = chdir(".");
	if (r < 0) {
		printf("r %d\n", r);
		return -1;
	}
	getcwd(buf, 256);
	printf("cwd %s\n\n", buf);

	printf("chdir ..\n");
	r = chdir("..");
	if (r < 0) {
		printf("r %d\n", r);
		return -1;
	}
	getcwd(buf, 256);
	printf("cwd %s\n\n", buf);

	printf("chdir /dir1/dir11\n");
	r = chdir("/dir1/dir11");
	if (r < 0) {
		printf("r %d\n", r);
		return -1;
	}
	getcwd(buf, 256);
	printf("cwd %s\n\n", buf);

	printf("chdir //\n");
	r = chdir("//");
	if (r < 0) {
		printf("r %d\n", r);
		return -1;
	}
	getcwd(buf, 256);
	printf("cwd %s\n", buf);

	return 0;
}

int test_stat() {
	int r;
	struct stat s;

	r = stat("/", &s);
	if (r < 0)
		return r;

	printf("/\n");
	printf("dev\t%d\n", (unsigned int)s.st_dev);
	printf("ino\t%d\n", (unsigned int)s.st_ino);
	printf("rdev\t%d\n", (unsigned int)s.st_rdev);
	printf("size\t%d\n", (unsigned int)s.st_size);
	printf("\n");

	r = stat("/dosya1", &s);
	if (r < 0)
		return r;
	printf("/dosya1\n");
	printf("dev\t%d\n", (unsigned int)s.st_dev);
	printf("ino\t%d\n", (unsigned int)s.st_ino);
	printf("rdev\t%d\n", (unsigned int)s.st_rdev);
	printf("size\t%d\n", (unsigned int)s.st_size);
	printf("\n");

	return 0;
}
