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

void parse_cmd(char *cmd, int *argc, char *argv[11]) {
	*argc = 0;
	argv[0] = cmd;

	/*
	 * 		cmd = " asd fff\t ggg"
	 * şeklindeki stringi aşağıdaki biçime dönüştürür
	 * 		cmd = "\0asd\0fff\0\0ggg"
	 *
	 * arg'lar, cmd'nin belirli bir karakterini referans eder
	 * 		argv[0] = "asd\0fff\0\0ggg"	-> &cmd[1]
	 * 		argv[1] = "fff\0\0ggg" 		-> &cmd[5]
	 * 		argv[2] = "ggg" 			-> &cmd[10]
	 * 		argc = 3
	 */
	for ( ; *cmd != '\0' ; cmd++) {
		switch (*cmd) {
			case ' ':
			case '\t':
			case '\n':
				*cmd = '\0'; /* boşluk karakterlerini null olarak işaretle */
				if ( *(cmd-1) != '\0' && (*argc) < 10) //10 -> max argc
					(*argc)++;
				argv[*argc] = cmd+1;
				break;
		}
	}

	if (argv[*argc][0] != '\0')
		(*argc)++;
	argv[*argc] = 0;
}
