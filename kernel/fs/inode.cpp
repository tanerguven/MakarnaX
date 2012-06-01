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

#include "../sched.h"
#include "fs.h"

void inode_lock(struct inode *inode) {
	pushcli();

	if (inode->lock.val > 0)
		inode->lock.val--;
	else
		sleep_uninterruptible(&inode->lock.wait_list);

	ASSERT(inode->lock.val == 0);
	popif();
}

void inode_unlock(struct inode *inode) {
	pushcli();

	ASSERT(inode->lock.val == 0);

	if (inode->lock.wait_list.size() > 0)
		wakeup_interruptible(&inode->lock.wait_list);
	else
		inode->lock.val++;

	popif();
}


struct inode *inode_alloc() {
	struct inode *inode;

	inode = (struct inode*)kmalloc(sizeof(struct inode));

	memset(inode, 0, sizeof(struct inode));

	inode->ref_count = 1;
	inode->lock.val = 1;
	inode->lock.wait_list.init();

	return inode;
}

struct inode *inode_dup(struct inode* inode) {
	if (inode == NULL)
		return NULL;
	inode_lock(inode);
	inode->ref_count++;
	inode_unlock(inode);
	return inode;
}

static void __inode_free(struct inode *inode) {
	print_info("__inode_free %08x\n", inode);

	ASSERT(inode->lock.wait_list.size() == 0);
	ASSERT(inode->ref_count == 0);

	kfree(inode);
}

void inode_free(struct inode *inode) {
	inode_lock(inode);

	if (--inode->ref_count < 0) {
		__inode_free(inode);
		return;
	}

	inode_unlock(inode);
}

int inode_permission(struct inode* inode, int flags) {
	if (inode->op->permission && inode->op->permission(inode, flags))
		return 1;
	if ((inode->mode.mode() & flags) == flags)
		return 1;
	return 0;
}

/** duplicated inode */
int inode_find(const char *path, size_t len, struct inode **inode) {
	struct dirent *dirent;
	int r;

	r = dirent_find(path, len, &dirent);
	if (r < 0)
		return r;

	*inode = inode_dup(dirent->inode);
	dirent_free(dirent);
	if (*inode == NULL)
		return -1;
	return 0;
}

/** duplicated i_parent */
int inode_find2(const char *path, struct inode **i_parent, const char **name) {
	struct dirent *d_parent;
	int r;

	r = dirent_find2(path, &d_parent, name);
	if (r < 0)
		return r;

	*i_parent = inode_dup(d_parent->inode);
	dirent_free(d_parent);
	if (*i_parent == NULL)
		return -1;
	return 0;
}
