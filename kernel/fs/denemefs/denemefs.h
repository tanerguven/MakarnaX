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

extern int denemefs_lookup(struct DirEntry*, const char*, uint32_t*);
extern int denemefs_read_super(struct SuperBlock* sb);

extern struct Deneme_inode* inode_to_deneme(struct inode *inode);

#endif /* _DENEMEFS_H_ */
