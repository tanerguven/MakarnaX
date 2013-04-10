#ifndef _FS_H
#define _FS_H

#include <kernel/kernel.h>

#define MAX_DIRENTRY_NAME_SIZE 256
#define MAX_PATH_SIZE 1536

#include "../task.h"

struct FileMode {
	uint32_t read: 1;
	uint32_t write: 1;
	uint32_t __: 6;
	uint32_t type: 8;

	enum {
		FT_regular = 1,
		FT_dir = 2,
		FT_symlink = 3,
		FT_chrdev = 4,
		FT_blkdev = 5,
		FT_fifo = 6,
	};

	inline uint16_t mode() { return (*(uint16_t*)this) & 0xFF; }

} __attribute__((packed));


struct SuperBlock {
	int dev;
	int fs_type;
	struct dirent *root;
};


struct inode {
	uint32_t ino;
	int ref_count;
	const struct inode_operations *op;
	FileMode mode;
	int dev;
	struct SuperBlock *superblock;
	uint32_t size;

	struct {
		TaskList_t wait_list;
		int val;
	} lock;
};


define_list(struct dirent, dirent_childs_t);

struct dirent {
	dirent_childs_t::node_t node_childs;

	struct spinlock lock;
	int ref_count;

	struct inode *inode;
	char name[MAX_DIRENTRY_NAME_SIZE];
	struct dirent *parent;
	dirent_childs_t childs;
	int mounted;
};


struct file {
	/* char path[MAX_PATH_SIZE]; */
	const struct file_operations *fo;
	struct inode *inode;
	int fpos;
	int flags;
};


struct file_operations {
	/** @return: okunan byte sayisi, -1: hata (?) */
	uint32_t (*read)(struct file *f, char *buf, size_t size);
	/** @return: yazilan byte sayisi, -1: hata (?) */
	uint32_t (*write)(struct file* f, const char *buf, size_t size);
	int (*readdir)(struct file *f, struct dirent_user *udirent);
	int (*open)(struct file *f);
	int (*release)(struct file *f);
};


struct inode_operations {

	const struct file_operations *default_file_ops;

	/* dir icerisinde name isimli dosyayi arar, dest ile dondurur */
	int (*lookup)(struct inode *i_dir, const char* name, struct inode *i_dest);
	int (*create)(struct inode *i_dir, const char* name, int rw, struct inode *i_dest);
	int (*unlink)(struct inode *i_dir, const char* name);
	int (*mkdir)(struct inode *i_dir, const char *name, int rw);
	int (*rmdir)(struct inode *i_dir, const char *name);
	int (*mknod)(struct inode *i_dir, const char *name, FileMode mode, int dev);
	int (*permission)(struct inode* i, int flags);
};


/* */
// FIXME: bunlar kullanici programlariyla ortak kullanilabilecek bir yerde olmali
typedef unsigned int mode_t;
typedef unsigned short nlink_t;
typedef short uid_t;
typedef short gid_t;
typedef long int blksize_t;
typedef long int blkcnt_t;
typedef long int time_t;
typedef long time_t;
struct stat
{
	dev_t st_dev;
	ino_t st_ino;
	mode_t st_mode;
	nlink_t st_nlink;
	uid_t st_uid;
	gid_t st_gid;
	dev_t st_rdev;
	off_t st_size;

	time_t st_atime;
	long st_spare1;
	time_t st_mtime;
	long st_spare2;
	time_t st_ctime;
	long st_spare3;
	long st_blksize;
	long st_blocks;
	long st_spare4[2];
};
/* */


// inode.cpp
extern struct inode *inode_dup(struct inode* inode);
extern struct inode *inode_alloc();
extern void inode_free(struct inode *inode);
extern void inode_unlock(struct inode *inode);
extern void inode_lock(struct inode *inode);
extern int inode_permission(struct inode* inode, int flags);
extern int inode_find(const char *path, size_t len, struct inode **inode);
extern int inode_find2(const char *path, struct inode **i_parent, const char **name);

// dirent.cpp
extern struct dirent *dirent_dup(struct dirent* dirent);
extern struct dirent *dirent_alloc();
extern void dirent_free(struct dirent *dirent);
extern int dirent_lookup(struct dirent *dir, struct dirent *name, struct dirent **d);
extern int dirent_topath(struct dirent *dirent, char *buf, size_t size);
extern int dirent_find(const char *path, size_t len, struct dirent **dirent);
extern int dirent_find2(const char *path, struct dirent **d_parent, const char **name);
extern struct dirent * dirent_cache_add(struct dirent *d_parent, const char *name, struct inode *i_child);
extern void dirent_cache_remove(const char *path);


// file.cpp
extern int file_open(const char *path, int flags, struct file **f);
extern int file_close(struct file *f);
extern int file_read(struct file *f, char *buf, size_t size);
extern int file_write(struct file *f, const char *buf, size_t size);
extern struct file* file_dup(struct file *f);
extern int unlink(const char *path, int file_type);
extern int create(const char *path, FileMode mode, int dev);
extern int file_stat(const char *path, struct stat *stat);
extern int file_stat_fd(int fd, struct stat *stat);
extern off_t file_lseek(int, off_t, int);

// task_fs.cpp
extern struct file * task_curr_get_file(int fd);

#endif /* _FS_H */
