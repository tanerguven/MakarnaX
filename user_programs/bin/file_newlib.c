#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#include <unistd.h>
#include <sys/stat.h>

#include "../test/include/test.h"

int main() {
	struct stat s;
	int r;

	printf("%d\n", offsetof(struct stat, st_mode));

	r = stat("/initrd/objdump", &s);

	ASSERT3(s.st_dev, ==, 123);
	printf("ino: %d\n", s.st_ino);
	ASSERT3(s.st_nlink, ==, 1);
	ASSERT3(s.st_mode, ==, 0100000);
	ASSERT3(s.st_uid, ==, 1000);
	ASSERT3(s.st_gid, ==, 1000);
	ASSERT3(s.st_rdev, ==, 0);
	printf("size: %d\n", s.st_size);

}
