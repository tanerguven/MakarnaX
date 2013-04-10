#include <kernel/syscall.h>

asmlink _syscall3(int, open, const char*, filename, int, flags, int, mode)
asmlink _syscall1(int, close, unsigned int, fd)

// FIXME: off_t
asmlink _syscall3(int, lseek, int, fd, int, offset, int, whence)


size_t read(unsigned int fd, char *buf, unsigned int count) {
	/* stdin yok, cgetc kullan */
	if (fd == 0) {
		int c;
		/* bir durumdan dolayi (alarm vb.) wake up olursa diye kontrol */
		while( (c = syscall(SYS_cgetc, 0, 0, 0, 0, 0)) == 0);
		buf[0] = c;
		return 1;
	}
	return (size_t)syscall(SYS_read, fd, (uint32_t)buf, count, 0, 0);
}

size_t write(int fd, char *buf, int nbytes) {
	/* stdout tanimli degil, cputs kullan */
	if (fd == 1 || fd == 2) {
		syscall(SYS_cputs, (uint32_t)buf, nbytes, 0, 0, 0);
		return nbytes;
	}
	return syscall(1000+fd, (uint32_t)buf, nbytes, 0, 0, 0);
}

// FIXME: int->mode_t
asmlink _syscall2(int, chmod, const char*, path, int, mode);

asmlink _syscall2(int, access, const char*, pathname, int, mode);
