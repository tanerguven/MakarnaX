
#ifndef _DIRENT_H
#define _DIRENT_H

struct dirent {
	ino_t          d_ino;       /* inode number */
	off_t          d_off;       /* offset to the next dirent */
	unsigned short d_reclen;    /* length of this record */
	unsigned char  d_type;      /* type of file; not supported
								   by all file system types */
	char           d_name[256]; /* filename */
};

typedef struct __DIR  {
	int fd;
	size_t count;
	struct dirent d[128];
} DIR;

#ifdef __cplusplus
extern "C" {
#endif

extern DIR *opendir(const char *path);
extern int closedir(DIR *dirp);
extern struct dirent* readdir(DIR *dirp);

#ifdef __cplusplus
}
#endif

#endif /* _DIRENT_H */
