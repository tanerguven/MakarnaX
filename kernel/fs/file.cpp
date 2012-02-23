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
#include "vfs.h"
#include <sys/stat.h>

// FIXME: path size

int do_open(File **f, const char *path) {
	DirEntry *file_dentry;
	int r;
	File *file;

	ASSERT(!(eflags_read() & FL_IF));

	r = find_dir_entry(path, &file_dentry);
	if (r < 0)
		return r;

	file = (File*)kmalloc(sizeof(File));

	strcpy(file->path, "/");
	strcpy(&file->path[1], path);
	file->inode = file_dentry->inode;
	file->fo = file->inode->op->default_file_ops;
	file->fpos = 0;
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
asmlink void sys_open() {
	Trapframe *tf = task_curr->registers();
	const char *path = (const char*)
		user_to_kernel_check(get_param1(tf), MAX_PATH_SIZE, 0);
	// int flags = (int)get_param2(tf);
	// int mode = (int)get_param3(tf);
	int fd;
	int r;
	File *f;

	uint32_t eflags = eflags_read();
	cli();

	r = do_open(&f, path);
	if (r < 0)
		return set_return(tf, r);

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
	return set_return(tf, fd);
}

asmlink void sys_close() {
	Trapframe *tf = task_curr->registers();
	unsigned int fd = get_param1(tf);

	uint32_t eflags = eflags_read();
	cli();

	if (task_curr->files[fd] == NULL)
		return set_return(tf, -1);

	do_close(task_curr->files[fd]);
	task_curr->files[fd] = NULL;

	eflags_load(eflags);
	return set_return(tf, 0);
}

int do_read(File *f, char *buf, unsigned int count) {
	int r;

	r = f->fo->read(f, buf, count);
	if (r < 0)
		return r;

	f->fpos += r;
	return r;
}

asmlink void sys_read() {
	Trapframe *tf = task_curr->registers();
	unsigned int fd = get_param1(tf);
	unsigned int count = get_param3(tf);
	char *buf = (char*)user_to_kernel_check(get_param2(tf), count, 1);
	size_t r;

	r = task_curr->files[fd]->fo->read(task_curr->files[fd], buf, count);

	// FIXME: bu islem nerede yapilmali ?
	task_curr->files[fd]->fpos += r;

	return set_return(tf, r);
}

asmlink void sys_readdir() {
	Trapframe *tf = task_curr->registers();
	// unsigned int fd = get_param1(tf);
	// // FIXME: adres kontrolu
	// struct dirent *dirent = (struct dirent*)get_param2(tf);
	// unsigned int count = get_param3(tf);

	PANIC("readdir tamamlanmadi\n");

	return set_return(tf, -1);
}

asmlink void sys_stat() {
	Trapframe *tf = task_curr->registers();
	char *path = (char*)user_to_kernel_check(get_param1(tf), MAX_PATH_SIZE, 1);
	struct stat* stat = (struct stat*)
		user_to_kernel_check(get_param2(tf),sizeof(struct stat), 1);
	struct DirEntry *dentry;
	int r;
	uint32_t eflags = eflags_read();
	cli();

	r = find_dir_entry(path, &dentry);
	if (r < 0)
		return set_return(tf, r);

	stat->st_dev = 123;
	stat->st_ino = dentry->inode->ino;
	stat->st_rdev = 0;
	stat->st_size =  dentry->inode->size;

	eflags_load(eflags);
	return set_return(tf, 0);
}

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

asmlink void sys_chdir() {
	Trapframe *tf = task_curr->registers();
	const char *path = (const char*)
		user_to_kernel_check(get_param1(tf), MAX_PATH_SIZE, 0);
	int r;
	DirEntry *dentry;

	uint32_t eflags = eflags_read();
	cli();

	r = find_dir_entry(path, &dentry);
	if (r < 0)
		return set_return(tf, r);

	task_curr->pwd = dentry;

	eflags_load(eflags);
	return set_return(tf, 0);
}

asmlink void sys_getcwd() {
	Trapframe *tf = task_curr->registers();
	int size = (int)get_param2(tf);
	char *buf = (char*)user_to_kernel_check(get_param1(tf), size, 1);

	dir_entry_to_path(task_curr->pwd, buf, size);

	return set_return(tf, 0);
}
