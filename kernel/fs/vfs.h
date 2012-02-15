#ifndef _VFS_H_
#define _VFS_H_

#include <types.h>
#include <wmc/list.h>

struct SuperBlock {
	int dev;
	int fs_type;
	struct DirEntry *root;
};

struct inode {
	uint32_t ino;
	SuperBlock *superblock;

	// uid, gid

	uint32_t size; // file size

	struct inode_operations *op;
	// last access time
	// last modify time
	// last change time
};

/**
 * directory entry, dizin ya da dosya
 */
struct DirEntry {

	int mounted;
	struct inode *inode;
	char name[256];

	inline void init() {
		mounted = 0;
	}
};

struct File {
	char path[1536];
	struct File_operations *fo;
	struct inode *inode;
	int fpos;
};

struct File_operations {
	uint32_t (*read)(struct File *f, char *buf, size_t size);
	int (*readdir)(struct File *f, struct dirent *dirent, uint32_t count);
	int (*open)(struct inode *inode, struct File *f);
	int (*release)(struct inode *inode, struct File *f);
};

struct inode_operations {

	/*
	 * dir icerisinde r->name isimli dosyayi arar, r icerisinde dondurur
	 */
	int (*lookup)(struct DirEntry *dir, struct DirEntry *r);
};


extern int lookup(struct DirEntry *dir, const char *fn, struct DirEntry **dentry);

#endif /* _VFS_H_ */
