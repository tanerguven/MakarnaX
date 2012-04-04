#ifdef _USER_PROGRAM
#include <user.h>
#include <genel_fonksiyonlar.h>
#else
#include <unistd.h>
#endif

#include <stdio.h>
#include <string.h>

int main(int argc, char** argv) {
	unsigned i;
	unsigned long dongu_sayisi = -1;
	int bekleme = 0;
	int r;
	int pid = getpid();

	if (argc > 1)
		bekleme = atoi(argv[1]);

	if (argc > 2)
		dongu_sayisi = atoi(argv[2]);

	for (i = 0 ; i < dongu_sayisi ; i++) {
		if (bekleme) {
			r = bekleme;
			while ((r = sleep(r)) > 0)
				printf("[%d] sleep kesildi\n", getpid());
		}

		printf("pid:%d, %d\n", pid, i);
	}

    return 0;
}
