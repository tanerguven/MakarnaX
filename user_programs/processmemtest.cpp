#include <user.h>
#include <stdio.h>
#include <asm/x86.h>

int stacktest();
int stacklimit();
int brktest();

struct {
	const char *name;
	const char *description;
	int (*function)();
} tests[] = {
	{"stacktest", "stacktest", stacktest},
	{"stacklimit", "stacklimit", stacklimit},
	{"brktest", "brktest", brktest},

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

int stacktest() {
	uint32_t esp; read_reg(%esp, esp);
	if (esp > 0xefff0000) {
		return stacktest();
	}
	printf("stacktest OK.\nesp: %08x\n", esp);
	return 123;
}

int stacklimit() {
	return stacklimit();
}


int brktest() {
	int r;

	void *start_brk;
	void *b;

	start_brk = sbrk(0);
	printf("brk = %08x\n", start_brk);

	r = brk(start_brk - 1);
	printf("brk(start-1) return: %d\n", r);

	r = brk(start_brk + 0x3000);
	printf("brk(start+0x3000)  return: %d\n", r);

	*((char*)start_brk+0x2000) = 'a';

	b = sbrk(0);
	printf("brk = %08x\n", b);

	r = brk(start_brk + 0x1000);
	printf("brk(start+0x1000) return: %d\n", r);

	*((char*)start_brk+0x800) = 'a';

	b = sbrk(0);
	printf("brk = %08x\n", b);

}
