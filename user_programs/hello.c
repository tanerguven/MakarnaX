#ifdef _USER_PROGRAM
#include <user.h>
#endif

#include <stdio.h>

int main(int argc, char** argv) {
	int i;

	printf("argc: %d\n", argc);
	for (i = 0 ; i < argc ; i++) {
		printf("argv[%d]: %s\n", i, argv[i]);
	}

	printf("Hello World!\n");
	printf("bir karakter girin:\n");
	int c = getchar();
	printf("karakter:");
	putchar(c);
	printf("\nbitti\n");

	return 0;
}
