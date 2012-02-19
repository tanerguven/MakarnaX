#include <types.h>

uint32_t intToString(char *buf, unsigned int num, unsigned int base) {
	int i = 0;
	int j;
	uint32_t size;

	do {
		buf[i++] = "0123456789abcdef"[num % base];
		num /= base;
	} while (num > 0);

	buf[i] = '\0';

	size = i;

	i--;
	for (j = 0; i > j; i--, j++) {
		char tmp = buf[i];
		buf[i] = buf[j];
		buf[j] = tmp;
	}

	return size;
}

uint32_t str_to_int(const char *s) {
	uint32_t  i = 0;
	while (*s != '\0') {
		i = i << 4;
		if ((*s >= 'a') && (*s <= 'f'))
			i += (*s - 'a') + 10;
		else if ((*s >= '0') && (*s <= '9'))
			i += *s - '0';
		else
			return i >> 4;
		s++;
	}
	return i;
}

int atoi(const char *s) {
	int i = 0;
	while (*s != '\0') {
		i = i*10;
		if ((*s >= '0') && (*s <= '9'))
			i += *s - '0';
		else
			return i/10;
		s++;
	}
	return i;
}
