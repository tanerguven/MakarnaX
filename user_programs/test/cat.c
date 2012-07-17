#include <stdio.h>
#include <string.h>

#include <user.h>
#include "include/test.h"

int main(int argc, char **argv) {
	int r;
	int fd;
	char buf[1536];

	strcpy(buf, argv[1]);

	if (argc != 2)
		return -1;

	fd = open(buf, 1, 0);
	ASSERT3(fd, >, -1);

	memset(buf, 0, sizeof(buf));
	r = read(fd, buf, 1536);
	ASSERT3(r, >, 0);

	printf("%s", buf);

	return 0;
}
