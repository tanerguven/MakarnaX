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

int file_open(const char *path, int flags, struct file **f) {

	// FIXME: okuma ve yazma izni secili degilse dosyayi acmanin mantigi yok. readonly ac.
	if (flags == 0)
		flags = 1;

	struct inode *file_inode;
	int r;
	struct file *file;

	ASSERT_int_disable();

	r = inode_find(path, -1, &file_inode);
	if (r < 0)
		return r;

	if ( ! inode_permission(file_inode, flags) )
		return -1; // FIXME: erisim izni hatasi

	file = (struct file*)kmalloc(sizeof(struct file));

	file->inode = inode_dup(file_inode);
	file->fo = file->inode->op->default_file_ops;
	file->fpos = 0;
	file->flags = flags;
	if (file->fo->open) {
		r = file->fo->open(file);
		if (r < 0)
			PANIC("FIXME");
	}

	*f = file;
	return r;
}

int file_close(struct file *f) {
	ASSERT(f->inode->ref_count > 0);

	if (f->fo->release)
		f->fo->release(f);

	inode_free(f->inode);
	kfree(f);

	return 0;
}

int file_read(struct file *f, char *buf, size_t size) {
	int r;

	if ((f->flags & 1) != 1)
		return -1; // FIXME: readonly file error

	r = f->fo->read(f, buf, size);
	if (r < 0)
		return -1;

	f->fpos += r;
	return r;
}

int file_write(struct file *f, const char *buf, size_t size) {
	int r;

	if ((f->flags & 2) != 2)
		return -1; // FIXME: readonly file error

	r = f->fo->write(f, buf, size);
	if (r < 0)
		return -1;

	f->fpos += r;
	return r;
}

struct file* file_dup(struct file *f) {
	struct file *file_new = (struct file*)kmalloc(sizeof(struct file));
	*file_new = *f;
	file_new->inode->ref_count++;
	return file_new;
}

int unlink(const char *path, int file_type) {
	struct inode *dir_inode;
	int r;
	const char *name;

	r = inode_find2(path, &dir_inode, &name);
	if (r < 0)
		return -1;

	inode_lock(dir_inode);

	if ( ! inode_permission(dir_inode, 3) ) {
		r = -1; // FIXME: erisim izni hatasi
		goto unlink_end;
	}

	if (file_type == FileMode::FT_regular)
		r = dir_inode->op->unlink(dir_inode, name);
	else if (file_type == FileMode::FT_dir)
		r = dir_inode->op->rmdir(dir_inode, name);
	if (r < 0)
		goto unlink_end;

	r = 0;
	dirent_cache_remove(path);

unlink_end:
	inode_unlock(dir_inode);
	return r;
}

int create(const char *path, FileMode fmode, int dev) {
	struct inode *i_dir;
	struct inode newinode;
	int r;
	const char *name;

	r = inode_find2(path, &i_dir, &name);
	if (r < 0)
		return -1;

	if ( ! inode_permission(i_dir, 3) )
		return -2; // FIXME: erisim izni hatasi

	if (fmode.type == FileMode::FT_regular)
		r = i_dir->op->create(i_dir, name, fmode.mode(), &newinode);
	else if (fmode.type == FileMode::FT_dir)
		r = i_dir->op->mkdir(i_dir, name, fmode.mode());
	else {
		print_info("create unknown file type: %d\n", fmode.type);
		return -4;
	}

	if (r < 0)
		return -3; // FIXME: error no
	return 0;
}

int file_stat_inode(struct inode *inode, struct stat *stat) {
	stat->st_dev = 123;
	stat->st_ino = inode->ino;
	stat->st_nlink = 1;
	stat->st_mode = 0100000; // regular file
	stat->st_uid = stat->st_gid = 1000;
	stat->st_rdev = 0;
	stat->st_size =  inode->size;

	stat->st_atime = stat->st_ctime = stat->st_mtime = 100000;

	stat->st_blksize =  512;
	stat->st_blocks =  inode->size / 512;

	return 0;
}

int file_stat(const char *path, struct stat *stat) {
	int r;
	struct inode *inode;

	r = inode_find(path, -1, &inode);
	if (r < 0)
		return -1;

	return file_stat_inode(inode, stat);
}

int file_stat_fd(int fd, struct stat *stat) {
	struct file *file;
	file = task_curr_get_file(fd);

	// FIXME: --
	if (fd == 1 || fd == 2 || fd == 0)
		return -1;

	print_info("file_stat_fd %d\n", fd);

	return file_stat_inode(file->inode, stat);
}

off_t file_lseek(int fd, off_t offset, int whence) {
	struct file *file;

	file = task_curr_get_file(fd);

	if (whence == 0) {
		// SEEK_SET
		file->fpos = offset;
	} else if (whence == 1) {
		// SEEK_CUR
		file->fpos += offset;
	} else if (whence == 2) {
		// SEEK_END
		file->fpos = file->inode->size - offset;
	} else {
		return -1;
	}

	return 0;
}
