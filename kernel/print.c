#include <stdarg.h>

#include <kernel/kernel.h>

// console.cpp
asmlink void console_putc(int c);

// lib/vsprintf.c
asmlink int vsprintf(char *buf, const char *fmt, va_list args);

int __kernel_print(const char *fmt, ...) {
	va_list args;
	int i;
	char buf[1024];

	va_start(args, fmt);
	i = vsprintf(buf, fmt, args);
	va_end(args);

	const char *c = buf;
	for (; *c != '\0' ; c++)
	    console_putc(*c);

	return i;
}
