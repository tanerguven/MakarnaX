#ifndef _DENEMEFS_H_
#define _DENEMEFS_H_

#include "../vfs.h"

struct Deneme_subdentry {
	int n;
	struct Deneme_inode *d[100];
};

struct Deneme_inode {
	enum FileType {
		FT_FILE = 1,
		FT_DIR = 2,
	};

	struct inode inode;
	char name[100];
	void *data;
	FileType ft;
};

extern int denemefs_lookup(struct DirEntry *dir, struct DirEntry *r);
extern int denemefs_read_super(struct SuperBlock* sb);

inline struct Deneme_inode* inode_to_deneme(struct inode *inode) {
	return (Deneme_inode*)inode;
}

inline struct inode * deneme_to_inode(struct Deneme_inode* di) {
	return (struct inode*)di;
}

#endif /* _DENEMEFS_H_ */
