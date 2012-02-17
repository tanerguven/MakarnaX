#ifndef _DENEMEFS_H_
#define _DENEMEFS_H_

#include "../vfs.h"

struct Deneme_subdentry {
	int n; // dentry count
	uint32_t no[16]; // dentry ino
	char name[16][32]; // dentry name
};

struct Deneme_inode {
	enum FileType {
		FT_FILE = 1,
		FT_DIR = 2,
	};
	FileType ft;
	size_t size;
	void *data;
};

extern struct Deneme_inode* inode_to_deneme(struct inode *inode);

/* inode operations */
extern int denemefs_lookup(struct inode *i_dir, const char* name, struct inode *i_dest);

/* file operations */
extern uint32_t denemefs_read(struct File *f, char *buf, size_t size);
extern int denemefs_open(struct File *f);
extern int denemefs_release(struct File *f);

extern int denemefs_read_super(struct SuperBlock* sb);

#endif /* _DENEMEFS_H_ */
