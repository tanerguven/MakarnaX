#ifndef _SYS_DIRENT_H
#define _SYS_DIRENT_H

typedef struct {
	int dd_fd; /* directory file */
	int dd_loc;
	int dd_seek;
	char *dd_buf;
	int dd_len;
	int dd_size;
} DIR;

#ifdef _KERNEL_SRC_
struct dirent_user {
# else
struct dirent {
#endif
	ino_t d_ino;
	off_t d_off;
	unsigned short d_reclen;
	unsigned char d_type;
	char d_name[256];
};

#endif /* _SYS_DIRENT_H */
