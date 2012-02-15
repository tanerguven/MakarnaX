
#include "vfs.h"
#include "../task.h"

#include <stdio.h>
#include <string.h>
#include "../kernel.h"

extern void denemefs_init();
extern void mount_root(struct Task*);

int lookup(struct DirEntry *dir, const char *fn, struct DirEntry **dentry) {

	ASSERT(dentry != NULL);

	/* root'da .. girilirse */
	if (fn[0] == '.' && fn[1] == '.' && fn[2] == '\n') {
		if (dir == task_curr->root) {
			*dentry = dir;
			return 0;
		}
	}

	*dentry = (struct DirEntry*)kmalloc(sizeof(DirEntry*));
	strcpy((*dentry)->name, fn);

	return dir->inode->op->lookup(dir, *dentry);
}

void init_vfs(Task* init_task) {
	denemefs_init();
	mount_root(init_task);
}
