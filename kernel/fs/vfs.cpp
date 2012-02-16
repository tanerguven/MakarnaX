
#include "vfs.h"
#include "../task.h"

#include <stdio.h>
#include <string.h>
#include "../kernel.h"

extern void denemefs_init();
extern void mount_root(struct Task*);

set_list_offset_2(DirEntry, Subdirs, node_subdirs);

int lookup(struct DirEntry *dir, const char *name, struct DirEntry **dentry) {
	int r;
	uint32_t no;
	ASSERT(dentry != NULL);

	/* root'da .. girilirse */
	if (name[0] == '.' && name[1] == '.' && name[2] == '\0') {
		if (dir == task_curr->root) {
			*dentry = dir;
			return 0;
		}
	}

	/* DirEntry cache icerisinde dosyayi ara */
	DirEntry::Subdirs::iterator it = dir->subdirs.begin();
	DirEntry::Subdirs::iterator end = dir->subdirs.end();
	for ( ; it != end ; it++) {
		if ( strcmp(it->value()->name, name) == 0) {
			*dentry = it->value();
			return 0;
		}
	}

	/* disk uzerinde dosyayi ara */
	r = dir->inode->op->lookup(dir, name, &no);
	if (r < 0)
		return r;
	/* diskte bulunan dosya icin DirEntry ve inode olustur */
	*dentry = (struct DirEntry*)kmalloc(sizeof(struct DirEntry*));
	(*dentry)->init();
	strcpy((*dentry)->name, name);
	(*dentry)->inode = (struct inode*)kmalloc(sizeof(struct inode));
	(*dentry)->inode->init(no, dir->inode->superblock, dir->inode->op);
	/* yeni DirEntry'i dir altina ekle */
	dir->add_subdir(*dentry);

	return 0;
}

void init_vfs(Task* init_task) {
	denemefs_init();
	mount_root(init_task);
}


struct File* dup_file(struct File *src) {
	struct File *f = (struct File*)kmalloc(sizeof(struct File));
	*f = *src;
	f->inode->ref_count++;
	return f;
}
