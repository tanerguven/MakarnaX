#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <sys/time.h>

int main() {
	int r;
	struct timeval tv;
	gettimeofday(&tv, NULL);
	srand(tv.tv_sec*1000 + tv.tv_usec/1000);

	r = rand();
	printf("%d\n", r);

	r = rand();
	printf("%d\n", r);

	r = rand();
	printf("%d\n", r);

	r = rand();
	printf("%d\n", r);

	return r;
}
