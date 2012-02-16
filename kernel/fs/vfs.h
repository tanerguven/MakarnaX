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
	int ref_count;
	struct inode_operations *op;

	inline void init(uint32_t ino, SuperBlock *sb, struct inode_operations *op) {
		this->ino = ino;
		this->superblock = sb;
		this->ref_count = 0;
		this->op = op;
	}
};

/**
 * directory entry, dizin ya da dosya
 */
struct DirEntry {
	define_list(DirEntry, Subdirs);
	Subdirs::node_t node_subdirs;

	int mounted;
	struct inode *inode;
	char name[256];

	struct DirEntry * parent;
	Subdirs subdirs;

	inline void init() {
		mounted = 0;
		parent = NULL;
		node_subdirs.init();
		subdirs.init();
	}

	inline void add_subdir(struct DirEntry *dir) {
		subdirs.push_back(&dir->node_subdirs);
		dir->parent = this;
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
	 * dir icerisinde name isimli dosyayi arar, *no ile ino dondurur
	 */
	int (*lookup)(struct DirEntry *dir, const char* name, uint32_t *no);
};


extern int lookup(struct DirEntry *dir, const char *fn, struct DirEntry **dentry);

#endif /* _VFS_H_ */
