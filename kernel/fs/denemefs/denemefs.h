#ifndef _DENEMEFS_H_
#define _DENEMEFS_H_

#include "../vfs.h"

#define Deneme_subdentry_count 16

struct Deneme_subdentry {
	int n; // dentry count
	uint32_t no[Deneme_subdentry_count]; // dentry ino
	char name[Deneme_subdentry_count][32]; // dentry name
};

struct Deneme_inode {
	FileMode mode;
	unsigned short nlinks;
	size_t size;
	int dev; // special file icin, dir ve regular'da 0
	void *data;
};

extern struct Deneme_inode* inode_to_deneme(struct inode *inode);

/* inode operations */
extern int denemefs_lookup(struct inode *i_dir, const char* name, struct inode *i_dest);
extern int denemefs_permission(struct inode *i, int flags);
extern int denemefs_create(struct inode *i_dir, const char* name, int mode, struct inode *i_dest);
extern int denemefs_unlink(struct inode *i_dir, const char* name);
extern int denemefs_mkdir(struct inode *i_dir, const char *name, int mode);
extern int denemefs_rmdir(struct inode *i_dir, const char *name);
extern int denemefs_mknod(struct inode *i_dir, const char *name, FileMode mode, int dev);
extern struct inode_operations denemefs_file_inode_op;
extern struct inode_operations denemefs_dir_inode_op;

/* file operations */
extern uint32_t denemefs_read(struct File *f, char *buf, size_t size);
extern uint32_t denemefs_write(struct File *f, const char *buf, size_t size);
extern int denemefs_open(struct File *f);
extern int denemefs_release(struct File *f);

extern int denemefs_read_super(struct SuperBlock* sb);

extern void denemefs_free_inode(struct inode *i);
extern int denemefs_new_inode(struct inode *i_dir, struct inode *dest);
extern int denemefs_add_entry(struct inode *dir, const char *name, struct inode *i);
extern int denemefs_remove_entry(struct inode *dir, const char *name);

extern struct Deneme_inode di[100];
int find_empty_di();

#endif /* _DENEMEFS_H_ */
