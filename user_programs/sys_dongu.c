#ifdef _USER_PROGRAM
#include <user.h>
#include <genel_fonksiyonlar.h>
#endif

#include <stdio.h>

int main(int argc, char** argv) {
	unsigned i, j;
	unsigned long dongu_sayisi = -1;
	uint32_t bekleme = 0;

	if (argc > 1)
		bekleme = atoi(argv[1]);

	if (argc > 2)
		dongu_sayisi = atoi(argv[2]);

	for (i = 0 ; i < dongu_sayisi ; i++) {
		for (j = 0; j < bekleme; j++);
		sys_dongu();
	}

    return 0;
}
