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

#include "../task.h"
#include "../kernel.h"
#include <kernel/syscall.h>
#include "vfs.h"
#include <sys/stat.h>

// FIXME: path size

int permission(struct inode* inode, int flags) {
	if (inode->op->permission && inode->op->permission(inode, flags))
		return 1;
	return 0;
}

int do_open(File **f, const char *path, int flags) {
	DirEntry *file_dentry;
	int r;
	File *file;

	ASSERT(!(eflags_read() & FL_IF));

	r = find_dir_entry(path, -1, &file_dentry);
	if (r < 0)
		return r;

	if ( ! permission(file_dentry->inode, flags) )
		return -1; // FIXME: erisim izni hatasi

	file = (File*)kmalloc(sizeof(File));

	strcpy(file->path, "/");
	strcpy(&file->path[1], path);
	file->inode = file_dentry->inode;
	file->fo = file->inode->op->default_file_ops;
	file->fpos = 0;
	file->flags = flags;
	file->fo->open(file);

	// FIXME: --
	file->inode->ref_count++;

	*f = file;
	return r;
}

void do_close(File *f) {
	ASSERT(f->inode->ref_count > 0);

	// FIXME: --
	f->inode->ref_count--;
	f->fo->release(f);

	kfree(f);
}

/* open, opendir cagrilari */
SYSCALL_DEFINE2(open, const char*, path, int, flags) {
	path = (const char*)user_to_kernel_check((uint32_t)path, MAX_PATH_SIZE, 0);
	int fd;
	int r;
	File *f;

	uint32_t eflags = eflags_read();
	cli();

	r = do_open(&f, path, flags);
	if (r < 0)
		return SYSCALL_RETURN(r);

	for (fd = 0 ; fd < TASK_MAX_FILE_NR ; fd++) {
		if (task_curr->files[fd] == NULL) {
			task_curr->files[fd] = f;
			// FIXME: --
			f->inode->ref_count++;
			break;
		}
	}

	if (fd == TASK_MAX_FILE_NR)
		PANIC("FIXME");

/*
 * TODO: opendir durumu ?
 */

	eflags_load(eflags);
	return SYSCALL_RETURN(fd);
}
SYSCALL_END(open)


SYSCALL_DEFINE1(close, unsigned int, fd) {
	uint32_t eflags = eflags_read();
	cli();

	if (task_curr->files[fd] == NULL)
		return SYSCALL_RETURN(-1);

	do_close(task_curr->files[fd]);
	task_curr->files[fd] = NULL;

	eflags_load(eflags);
	return SYSCALL_RETURN(0);
}
SYSCALL_END(close)


int do_read(File *f, char *buf, size_t size) {
	int r;

	if ((f->flags & 1) != 1)
		return -1; // FIXME: readonly file error

	r = f->fo->read(f, buf, size);
	if (r < 0)
		return -1;

	f->fpos += r;
	return r;
}

SYSCALL_DEFINE3(read, unsigned int, fd, char*, buf, size_t, size) {
	buf = (char*)user_to_kernel_check((uint32_t)buf, size, 1);
	size_t r;

	// TODO: olmayan dosya girilirse hata ver
	r = do_read(task_curr->files[fd], buf, size);
	return SYSCALL_RETURN(r);
}
SYSCALL_END(read)


int do_write(File *f, const char *buf, size_t size) {
	int r;

	if ((f->flags & 2) != 2)
		return -1; // FIXME: readonly file error

	r = f->fo->write(f, buf, size);
	if (r < 0)
		return -1;

	f->fpos += r;
	return r;
}

SYSCALL_DEFINE3(write, unsigned int, fd, const char*, buf, size_t, size) {
	buf = (const char*)user_to_kernel_check((uint32_t)buf, size, 1);
	size_t r;

	// TODO: olmayan dosya girilirse hata ver
	r = do_write(task_curr->files[fd], buf, size);
	return SYSCALL_RETURN(r);
}
SYSCALL_END(write)

SYSCALL_DEFINE0(readdir) {
	// unsigned int fd = get_param1(tf);
	// // FIXME: adres kontrolu
	// struct dirent *dirent = (struct dirent*)get_param2(tf);
	// unsigned int count = get_param3(tf);

	PANIC("readdir tamamlanmadi\n");

	return SYSCALL_RETURN(-1);
}
SYSCALL_END(readdir)

SYSCALL_DEFINE2(stat, char*, path, struct stat*, stat) {
	path = (char*)user_to_kernel_check((uint32_t)path, MAX_PATH_SIZE, 1);
	stat = (struct stat*)user_to_kernel_check((uint32_t)stat,sizeof(struct stat), 1);

	struct DirEntry *dentry;
	int r;
	uint32_t eflags = eflags_read();
	cli();

	r = find_dir_entry(path, -1, &dentry);

	if (r < 0)
		return SYSCALL_RETURN(r);

	stat->st_dev = 123;
	stat->st_ino = dentry->inode->ino;
	stat->st_rdev = 0;
	stat->st_size =  dentry->inode->size;

	eflags_load(eflags);
	return SYSCALL_RETURN(0);
}
SYSCALL_END(stat)

struct File* dup_file(struct File *src) {
	struct File *f = (struct File*)kmalloc(sizeof(struct File));
	*f = *src;
	f->inode->ref_count++;
	return f;
}

/* task_curr'in tum dosyalarini kapatir (task_free'de kullanim icin) */
void task_curr_free_files() {
	for (int i = 0 ; i < TASK_MAX_FILE_NR ; i++) {
		if (task_curr->files[i]) {
			do_close(task_curr->files[i]);
			task_curr->files[i] = NULL;
		}
	}
}

SYSCALL_DEFINE1(chdir, const char*, path) {
	path = (const char*)user_to_kernel_check((uint32_t)path, MAX_PATH_SIZE, 0);
	int r;
	DirEntry *dentry;

	uint32_t eflags = eflags_read();
	cli();

	r = find_dir_entry(path, -1, &dentry);
	if (r < 0)
		return SYSCALL_RETURN(r);

	task_curr->pwd = dentry;

	eflags_load(eflags);
	return SYSCALL_RETURN(0);
}
SYSCALL_END(chdir)

SYSCALL_DEFINE2(getcwd, char*, buf, int, size) {
	buf = (char*)user_to_kernel_check((uint32_t)buf, size, 1);

	dir_entry_to_path(task_curr->pwd, buf, size);

	return SYSCALL_RETURN(0);
}
SYSCALL_END(getcwd)


int do_mkdir(const char *path, int mode) {
	DirEntry *dir;
	int r;
	const char *name;

	ASSERT(!(eflags_read() & FL_IF));

	r = find_file_and_dir(path, &dir, &name);
	if (r < 0)
		return -1;

	if ( ! permission(dir->inode, 3) )
		return -2; // FIXME: erisim izni hatasi

	r = dir->inode->op->mkdir(dir->inode, name, mode);
	if (r < 0)
		return -3;

	return 0;
}


SYSCALL_DEFINE2(mkdir, const char*, path, int, mode) {
	path = (const char*)user_to_kernel_check((uint32_t)path, MAX_PATH_SIZE, 0);
	int r;

	uint32_t eflags = eflags_read();
	cli();

	r = do_mkdir(path, mode);

	eflags_load(eflags);
	return SYSCALL_RETURN(r);
}
SYSCALL_END(mkdir)


int do_rmdir(const char *path) {
	DirEntry *dir;
	int r;
	const char *name;

	ASSERT(!(eflags_read() & FL_IF));

	r = find_file_and_dir(path, &dir, &name);
	if (r < 0)
		return -1;

	if ( ! permission(dir->inode, 3) )
		return -1; // FIXME: erisim izni hatasi

	r = dir->inode->op->rmdir(dir->inode, name);
	if (r < 0)
		return -1;

	remove_from_dir_entry_cache(path);

	return 0;
}

SYSCALL_DEFINE1(rmdir, const char*, path) {
	path = (const char*)user_to_kernel_check((uint32_t)path, MAX_PATH_SIZE, 0);
	int r;

	uint32_t eflags = eflags_read();
	cli();

	r = do_rmdir(path);

	eflags_load(eflags);
	return SYSCALL_RETURN(r);
}
SYSCALL_END(rmdir)


int do_creat(const char *path, int mode) {
	DirEntry *dir;
	int r;
	const char *name;
	struct inode newinode;

	ASSERT(!(eflags_read() & FL_IF));

	r = find_file_and_dir(path, &dir, &name);
	if (r < 0)
		return -1;

	if ( ! permission(dir->inode, 3) )
		return -2; // FIXME: erisim izni hatasi

	r = dir->inode->op->create(dir->inode, name, mode, &newinode);
	if (r < 0)
		return -3;

	return 0;
}

SYSCALL_DEFINE2(creat, const char*, path, int, mode) {
	path = (const char*)user_to_kernel_check((uint32_t)path, MAX_PATH_SIZE, 0);
	int r;

	uint32_t eflags = eflags_read();
	cli();

	r = do_creat(path, mode);

	eflags_load(eflags);
	return SYSCALL_RETURN(r);
}
SYSCALL_END(creat)

int do_unlink(const char *path) {
	path = (const char*)user_to_kernel_check((uint32_t)path, MAX_PATH_SIZE, 0);
	DirEntry *file_dentry;
	int r;
	const char *name;

	ASSERT(!(eflags_read() & FL_IF));

	r = find_file_and_dir(path, &file_dentry, &name);
	if (r < 0)
		return -1;

	if ( ! permission(file_dentry->inode, 3) )
		return -1; // FIXME: erisim izni hatasi

	r = file_dentry->inode->op->unlink(file_dentry->inode, name);
	if (r < 0)
		return -1;

	remove_from_dir_entry_cache(path);

	return 0;
}

SYSCALL_DEFINE1(unlink, const char*, path) {
	path = (const char*)user_to_kernel_check((uint32_t)path, MAX_PATH_SIZE, 0);
	int r;

	uint32_t eflags = eflags_read();
	cli();

	r = do_rmdir(path);

	eflags_load(eflags);
	return SYSCALL_RETURN(r);
}
SYSCALL_END(rmdir)
