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

#include "test.h"

int openfile();
int readfile();
int writefile();
int test_readdir();
int test_chdir();
int test_stat();
int mkdir_rmdir();
int creat_unlink();

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
	{"writefile", "writefile", writefile},
	{"mkdir_rmdir", "mkdir_rmdir", mkdir_rmdir},
	{"creat_unlink", "creat_unlink", creat_unlink},
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

	r = open("dosya1", 1, 0);
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
	fd1 = open("dosya1", 1, 0);
	printf("fd1: %d\n", fd1);
	if (fd1 < 0)
		return fd1;

	r = read(fd1, buf, 10);
	printf("r %d\n", r);
	printf("buf: %s\n", buf);

	/* forktest programini dosya olarak ac */
	printf("\nopen forktest\n");
	fd2 = open("/bin/forktest", 1, 0);
	printf("fd2: %d\n", fd2);
	if (fd2 < 0)
		return fd2;

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
	fd1 = open("dosya2", 1, 0);
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

int writefile() {
	int r;
	int fd1, fd2;
	char buf[1000];
	const char *yazi = "dosyaya yazma deneme";

	/*
	 * dosya1 isimli dosyayi 2 kere ro ac ve birinde yazip digerinde oku
	 * dosya readonly oldugu icin yazmamali
	 */
	printf("open dosya1 R\n");
	fd1 = open("dosya1", 1, 0);
	printf("fd1: %d\n", fd1);
	if (fd1 < 0)
		return fd1;

	fd2 = open("dosya1", 1, 0);
	printf("fd2: %d\n", fd2);
	if (fd2 < 0)
		return fd2;

	r = write(fd1, yazi, strlen(yazi));
	printf("r %d\n", r);
	// TODO: assert(r == -1);

	r = read(fd2, buf, 10);
	printf("r %d\n", r);
	printf("buf: %s\n", buf);
	// TODO: assert(r == 7);
	// TODO: assert(buf == data 1);

	close(fd1);
	close(fd2);
	printf("ro file write error OK\n");

	printf("\n");

	/*
	 * dosya2 isimli dosyayi rw ve ro ac
	 * dosya rw oldugu icin yazilabilmeli
	 */
	printf("open dosya2 W\n");
	fd1 = open("dosya2", 2, 0);
	printf("fd1: %d\n", fd1);
	if (fd1 < 0)
		return fd1;

	fd2 = open("dosya2", 1, 0);
	printf("fd2: %d\n", fd2);
	if (fd2 < 0)
		return fd2;

	r = write(fd1, yazi, strlen(yazi));
	printf("r %d\n", r);
	// TODO: assert(r == -1);

	r = read(fd2, buf, -1);
	printf("r %d\n", r);
	printf("buf: %s\n", buf);
	// TODO: assert(r == 100);
	// TODO: assert(buf == yazi);

	printf("rw file write OK\n");
	close(fd1);
	close(fd2);

	printf("\n");

	/* dosya1'i w ac */
	printf("open dosya1 W\n");
	fd1 = open("dosya1", 2, 0);
	printf("fd1: %d\n", fd1);
	//TODO: assert(fd1 > 0)

	/* dosya1'i rw ac */
	printf("open dosya1 RW\n");
	fd1 = open("dosya1", 2, 0);
	printf("fd1: %d\n", fd1);
	//TODO: assert(fd1 < 0)

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

int mkdir_rmdir() {
	int r;

	/* / ro, home silinemez */
	r = rmdir("/home");
	assertNotEquals(0, r);

	/* /home/dir2 olustur ve sil */
	r = mkdir("/home/dir2", 1);
	assertNotLess(r, 0);
	r = rmdir("/home/dir2");
	assertNotLess(r, 0);

	/* /home/dir2 silindi, erisilemez */
	r = chdir("/home/dir2");
	assertNotEquals(0, r);

	/* /home/dir2 tekrar olustur, chdir ve sil */
	r = mkdir("/home/dir2", 1);
	assertNotLess(r, 0);
	r = chdir("/home/dir2");
	assertNotLess(r, 0);
	r = rmdir("/home/dir2");
	assertNotLess(r, 0);

	/* silinmis /home/dir2 ye chdir ve sil */
	r = chdir("/home/dir2");
	assertNotEquals(0, r);
	r = rmdir("/home/dir2");
	assertNotEquals(0, r);

	/* ic ice dizin */
	r = mkdir("/home/dir1", 3);
	assertNotLess(r, 0);
	r = mkdir("/home/dir1/dir11", 1);
	assertNotLess(r, 0);

	/* ro dizine (/home/dir1/dir11), dizin olusturlamaz */
	r = mkdir("/home/dir1/dir11/dir111", 1);
	assertNotEquals(0, r);

	/* ayni isimde dizinler */
	r = mkdir("/home/home", 3);
	assertNotLess(r, 0);
	r = mkdir("/home/home/dir1", 3);
	assertNotLess(r, 0);
	r = rmdir("/home/home/dir1");
	assertNotLess(r, 0);

	/* // FIXME: bos olmayan dizinler silinmemeli */
	/* r = rmdir("/home/dir1"); */
	/* asssertNotEquals(0, r); */

	printf("mkdir_rmdir OK\n");

	return 0;
}

int creat_unlink() {
	int r;
	int fd;
	char buf[256];

	r = creat("/home/deneme.txt", 3);
	if (r < 0)
		printf("hata %s:%d\n", __FILE__, __LINE__);

	/* olusturulan dosyaya birseyler yaz */
	fd = open("/home/deneme.txt", 3, 0);
	assertNotLess(fd, 0);
	r = write(fd, "deneme", 6);
	assertEquals(6, r);
	r = close(fd);
	assertNotLess(r, 0);

	/* olusturulan dosyayi ro modunda ac */
	fd = open("/home/deneme.txt", 1, 0);
	assertNotLess(r, 0);

	/* yazmaya calis */
	r = write(fd, "aaaaa", 5);
	assertNotEquals(5, r);
	/* oku */
	r = read(fd, buf, 256);
	assertEquals(256, r);
	assertEquals(0, strcmp(buf, "deneme"));
	r = close(fd);
	assertNotLess(r, 0);

	/* sil */
	r = unlink("/home/deneme.txt");
	assertNotLess(r, 0);

	/* silinmis dosyayi acmaya calis */
	fd = open("/home/deneme.txt", 1, 0);
	assertNotLess(-1, fd);

	/* ro dosya olustur */
	r = creat("/home/ro.txt", 1);
	assertNotLess(r, 0);

	/* ro dosyayi rw olarak acmaya calis */
	fd = open("/home/ro.txt", 3, 0);
	assertNotLess(r, 0);

	/* ro dosyayi sil */
	r = unlink("/home/ro.txt");
	assertNotLess(r, 0);

	/* silinmis ro dosyayi acmaya calis */
	fd = open("/home/ro.txt", 1, 0);
	assertNotLess(-1, fd);

	return 0;
}
