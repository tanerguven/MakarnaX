#include <stdarg.h>
#include <user.h>

// FIXME: --
#include "../user_programs/test/include/stdio.h"

static char buf[1024];

int printf(const char *fmt, ...) {
	va_list args;
	int i;

	va_start(args, fmt);
	i = vsprintf(buf, fmt, args);
	va_end(args);

	sys_cputs(buf, -1);

	return i;
}

int getchar() {
	return sys_cgetc();
}

void putchar(int c) {
	printf("%c", c);
}

int iscons(int fdnum) {
	return 1;
}
