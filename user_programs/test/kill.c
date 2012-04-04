#ifdef _USER_PROGRAM
#include <user.h>
#include <genel_fonksiyonlar.h>
#else
#include <unistd.h>
#endif

#include <stdio.h>
#include <string.h>
#include <signal.h>

int main(int argc, char** argv) {
	unsigned r;
	int pid;

	if (argc > 1)
		pid = atoi(argv[1]);
	else
		return 1;

	printf("kill %d\n", pid);
	r = kill(pid, SIGTERM);
	if (r < 0)
		printf("sys_kill err: %d\n", r);

    return 0;
}
