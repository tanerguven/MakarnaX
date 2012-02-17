/*
 * Copyright (C) 2012 Taner Guven <tanerguven@gmail.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

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
	struct inode *inode = (struct inode*)kmalloc(sizeof(struct inode));
	r = dir->inode->op->lookup(dir->inode, name, inode);
	if (r < 0) {
		kfree(inode);
		return r;
	}
	/* diskte bulunan dosya icin DirEntry ve inode olustur */
	*dentry = (struct DirEntry*)kmalloc(sizeof(struct DirEntry*));
	(*dentry)->init();
	strcpy((*dentry)->name, name);
	(*dentry)->inode = inode;
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
