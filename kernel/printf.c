#include <stdio.h>
#include <stdarg.h>

int printf(const char *fmt, ...) {
	va_list args;
	int i;
	char buf[1024];

	va_start(args, fmt);
	i = vsprintf(buf, fmt, args);
	va_end(args);

	const char *c = buf;
	for (; *c != '\0' ; c++)
	    putchar(*c);

	return i;
}
