#ifdef _USER_PROGRAM
#include <user.h>
#endif

#include <stdio.h>

int main(int argc, char** argv) {

	printf("HATA!\n"
		   "user_programs/bin icerisindeki program derlenmemis.\n"
		   "tools derlenmemis olabilir\n"
		   "duzeltmek icin:\n"
		   "  make clean clean-tools all-tools"
		   "\n\n");

	return 0;
}
