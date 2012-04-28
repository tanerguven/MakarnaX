#include <user.h>
#include <stdio.h>

char buf[2048];
char *lines[100];

int main(int argc, char** argv) {
	int fd, pid;
	int size;
	int i, i_line = 0;

	fd = open("/init_script", 1, 0);
	if (fd < 0) {
		printf("/init_script not found\n");
		return -1;
	}

	size = read(fd, buf, 2048);
	if (size < 0) {
		printf("/init_script okunamadi\n");
		return -1;
	}

	printf("init_script:\n");
	/* dosyayi satir satir ayir */
	lines[0] = buf;
	for (i = 0 ; i <= size ; i++) {
		if ( (buf[i] == '\n') || (i == size) ) {
			buf[i] = '\0';
			printf("  %s\n", lines[i_line]);
			i_line++;
			if (i != size)
				lines[i_line] = &buf[i+1];
		}
	}

	/* her satir icin bir proses olustur */
	for (i = 0 ; i < i_line ; i++) {

		if (strcmp(lines[i], "")==0)
			continue;

		pid = fork();

		if (pid < 0) {
			printf("[init] fork yapilamadi\n");
		} else if (pid == 0) {
			char *argv[11];
			int argc;
			parse_cmd(lines[i], &argc, argv);
			execve(argv[0], argv);
			printf("[%d] execve error\n", getpid());
			exit(0);
		}
	}

    while (1) {
		pause();
	}
}
