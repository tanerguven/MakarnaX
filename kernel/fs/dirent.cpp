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

#include <kernel/kernel.h>

#include "fs.h"

set_list_offset(struct dirent, dirent_childs_t, node_childs);

struct dirent *dirent_alloc() {
	struct dirent *d = (struct dirent*)kmalloc(sizeof(struct dirent));

	spinlock_init(&d->lock);
	d->ref_count = 1;
	d->mounted = 0;
	d->parent = NULL;
	d->node_childs.init();
	d->childs.init();
	return d;
}

struct dirent *dirent_dup(struct dirent* dirent) {
	spinlock_acquire(&dirent->lock);
	dirent->ref_count++;
	spinlock_release(&dirent->lock);
	return dirent;
}

static void __dirent_free(struct dirent *dirent) {
	ASSERT(dirent->ref_count == 0);
	ASSERT(dirent != task0.fs.root);
	ASSERT(dirent->parent);
	ASSERT(dirent->parent->lock.locked);
	ASSERT(dirent->lock.locked);
	kfree(dirent);
}

/**
 * bir process tarafindan birakilirken kullanilan
 *
 * dirent_find gibi fonksiyonlarda dup yapiliyor.
 * dup yapilan fonksiyonlar kullanildiktan sonra dirent_free kullanilmali
 */
void dirent_free(struct dirent *dirent) {
	pushcli();
	if (--dirent->ref_count < 0)
		__dirent_free(dirent);
	popcli();
}

/** cache uzerinden silerken kullanilan */
static void dirent_delete(struct dirent *dir) {
	ASSERT(dir->parent->lock.locked);

	spinlock_acquire(&dir->lock);
	while ( dir->childs.size() > 0 ) {
		struct dirent *d = dir->childs.front();
		ASSERT( dir->childs.pop_front() );
		dirent_delete(d);
	}

	dir->inode = NULL;
	dir->parent = NULL;
	spinlock_release(&dir->lock);
	dirent_free(dir);
}

/** duplicated d */
int dirent_lookup(struct dirent *dir, const char *name, struct dirent **d) {
	int r;
	ASSERT(d != NULL);

	if (dir->inode == NULL)
		return -1; // FIXME: error no, silinmis dirent

	pushcli();
	*d = NULL;

	/* . veya .. */
	if (mem_equals_2(name, ".\0")) {
		*d = dirent_dup(dir);
	} else if (mem_equals_3(name, "..\0")) {
		if (dir == task_curr->fs.root)
			*d = dirent_dup(dir);
		else
			*d = dirent_dup(dir->parent);
		ASSERT(*d != NULL);
	}
	if(*d) {
		popcli();
		return 0;
	}

/*
 * dirent cache icerisinde ara
 */
	dirent_childs_t::iterator it = dir->childs.begin();
	dirent_childs_t::iterator end = dir->childs.end();
	for ( ; it != end ; it++) {
		if ( strcmp(it->value()->name, name) == 0) {
			*d = dirent_dup(it->value());
			popcli();
			return 0;
		}
	}

/*
 * cachede yok, disk uzerinde ara
 * bulunca diskte bulunan dosya icin dirent ve inode olustur
 */
	spinlock_acquire(&dir->lock);
	inode_lock(dir->inode);
	popcli();
	struct inode *inode = inode_alloc();
	r = dir->inode->op->lookup(dir->inode, name, inode);
	if (r < 0)
		inode_free(inode);
	else
		*d = dirent_dup( dirent_cache_add(dir, name, inode) );

	inode_unlock(dir->inode);
	spinlock_release(&dir->lock);
	return r;
}

int dirent_topath(struct dirent *dirent, char *buf, size_t size) {
	pushcli();

	struct dirent *dir_stack[100];
	int i = 0; // dir_stack count
	int j = 1; // buf count

	while (dirent->parent != NULL) {
		dir_stack[i] = dirent;
		i++;
		dirent = dirent->parent;
	}

	strcpy(buf, "/");

	while ((--i > -1) && (size > 0)) {
		int len = strlen(dir_stack[i]->name);
		strncpy(&buf[j], dir_stack[i]->name, size-1);
		size -= len+1;
		j += len+1;
		if (i > 0)
			buf[j-1] = '/';
		else
			buf[j-1] = '\0';
	}

	popcli();
	return j;
}

/** duplicated d */
int dirent_find(const char *path, size_t len, struct dirent **d) {
	int i = 0, r = 0;
	char buf[MAX_DIRENTRY_NAME_SIZE];

	struct dirent *curr, *next;

	if (path[0] == '/')
		curr = dirent_dup(task_curr->fs.root);
	else
		curr = dirent_dup(task_curr->fs.pwd);

	do {
		i = parse_path_i(path, len, i, buf);
		if (buf[0] == '\0')
			continue;
		r = dirent_lookup(curr, buf, &next);
		dirent_free(curr);
		if (r < 0)
			return r;
		curr = next;
	} while (i > 0);

	*d = curr;
	return r;
}

/** duplicated d */
int dirent_find2(const char *path, struct dirent **d_parent, const char **name) {
	int r;
	const char *s = strrchr(path, '/');

	if (s == NULL) {
		*name = path;
		*d_parent = task_curr->fs.pwd;
	} else {
		*name = s+1;
		r = dirent_find(path, (size_t)(s - path), d_parent);
		if (r < 0)
			return -1;
	}

	return 0;
}

void dirent_cache_remove(const char *path) {
	struct dirent *dir, *dirent = NULL;
	const char *name;
	int r;

	r = dirent_find2(path, &dir, &name);
	if (r < 0)
		return;

	spinlock_acquire(&dir->lock);

	/* childlar arasindan name isimliyi bul ve sil */
	dirent_childs_t::iterator it = dir->childs.begin();
	dirent_childs_t::iterator end = dir->childs.end();
	for ( ; it != end ; it++) {
		if ( strcmp(it->value()->name, name) == 0) {
			dirent = it->value();
			ASSERT( dir->childs.erase(it) != dir->childs.error() );
			dirent_delete(dirent);
			break;
		}
	}

	spinlock_release(&dir->lock);
	dirent_free(dir);
}

struct dirent* dirent_cache_add(struct dirent *d_parent, const char *name, struct inode *i_child) {
	ASSERT(d_parent->inode->lock.val == 0);

	struct dirent *d_child = dirent_alloc();
	strcpy(d_child->name, name);
	d_child->inode = i_child;

	d_parent->childs.push_back(&d_child->node_childs);
	d_child->parent = d_parent;

	return d_child;
}
