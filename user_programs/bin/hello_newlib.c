#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void test2();

int main(int argc, char** argv) {
	int i;
	int c;

	printf("argc: %d\n", argc);
	for (i = 0 ; i < argc ; i++) {
		printf("argv[%d]: %s\n", i, argv[i]);
	}

	printf("Hello World!\n");
	printf("bir karakter girin:\n");

	c = getchar();
	printf("karakter:");
	putchar(c);
	printf("\nbitti\n");

	test2();

	return 0;
}

void test2() {
	int c;
	char *x, *y;

	printf("newlib malloc test\n");

	x = malloc(0x5000);
	memset(x, 1, 0x5000);
	printf("x: %p\n", x);

	y = malloc(0x100000);
	memset(y, 1, 0x10000);
	printf("y: %p\n", y);

	x = realloc(x, 0x100000);
	printf("x: %p\n", x);

	strcpy(x, "asd");
	free(x);
	free(y);

	y = malloc(0x1000000);
	memset(y, 1, 0x1000000);
	printf("y: %p\n", y);

	printf("devam etmek icin bir tusa basin\n");
	c = getchar();
	free(y);

	printf("devam etmek icin bir tusa basin\n");
	c = getchar();

	printf("newlib malloc test OK\n");
}
