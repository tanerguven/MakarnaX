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
#include <kernel/syscall.h>
#include <sys/dirent.h>

#include "fs.h"
#include "../task.h"

SYSCALL_DEFINE2(open, const char*, path, int, flags) {
	// FIXME: max length
	path = (char*)user_to_kernel_check((uint32_t)path, -1, 0);

	int fd;
	int r;
	file *f;

	pushcli();

	r = file_open(path, flags, &f);
	if (r < 0) {
		popcli();
		return SYSCALL_RETURN(r);
	}

	// FIXME: 0,1,2 stdin,out ve err icin sabit olarak atandi. Boyle olmamali
	for (fd = 3 ; fd < TASK_MAX_FILE_NR ; fd++) {
		if (task_curr->fs.files[fd] == NULL) {
			print_info(">> opened fd:%d\n", fd);
			task_curr->fs.files[fd] = f;
			// FIXME: --
			f->inode->ref_count++;
			break;
		}
	}

	if (fd == TASK_MAX_FILE_NR) {
		print_info(">> task maximum file count exceeded (32)");
		fd = -1;
	}

	popcli();
	return SYSCALL_RETURN(fd);
}
SYSCALL_END(open)


SYSCALL_DEFINE1(close, unsigned int, fd) {
	pushcli();

	if (task_curr->fs.files[fd] == NULL) {
		popif();
		return SYSCALL_RETURN(-1);
	}

	file_close(task_curr->fs.files[fd]);
	task_curr->fs.files[fd] = NULL;

	popif();
	return SYSCALL_RETURN(0);
}
SYSCALL_END(close)


SYSCALL_DEFINE3(read, unsigned int, fd, char*, buf, size_t, size) {
	buf = (char*)user_to_kernel_check((uint32_t)buf, size, 1);
	size_t r;

	print_info(">> read_file: %d %d\n", fd, size);

	// TODO: olmayan dosya girilirse hata ver
	r = file_read(task_curr->fs.files[fd], buf, size);
	print_info(">> read ok %d\n", r);
	return SYSCALL_RETURN(r);
}
SYSCALL_END(read)


SYSCALL_DEFINE3(write, unsigned int, fd, const char*, buf, size_t, size) {
	buf = (const char*)user_to_kernel_check((uint32_t)buf, size, 1);
	size_t r;

	// TODO: olmayan dosya girilirse hata ver
	r = file_write(task_curr->fs.files[fd], buf, size);
	return SYSCALL_RETURN(r);
}
SYSCALL_END(write)


SYSCALL_DEFINE2(creat, const char*, pathname, int, mode) {
	int r;
	pathname = (const char*)user_to_kernel_check((uint32_t)pathname, MAX_PATH_SIZE, 0);

	FileMode fmode;
	fmode.read = mode;
	fmode.write = mode >> 1;
	fmode.type = FileMode::FT_regular;

	r = create(pathname, fmode, -1);
	return SYSCALL_RETURN(r);
}
SYSCALL_END(creat)


SYSCALL_DEFINE1(unlink, const char*, path) {
	int r;
	path = (const char*)user_to_kernel_check((uint32_t)path, MAX_PATH_SIZE, 0);

	r = unlink(path, FileMode::FT_regular);
	return SYSCALL_RETURN(r);
}
SYSCALL_END(unlink)

SYSCALL_DEFINE2(stat, char*, path, struct stat*, stat) {
	int r;
	// FIXME: max length
	path = (char*)user_to_kernel_check((uint32_t)path, -1, 0);
	stat = (struct stat*)user_to_kernel_check((uint32_t)stat, sizeof(struct stat), 1);

	r = file_stat(path, stat);
	print_info("stat ok: %d\n", r);

	return SYSCALL_RETURN(r);
}
SYSCALL_END(stat)

SYSCALL_DEFINE2(fstat, int, fd, struct stat*, buf) {
	int r;
	buf = (struct stat*)user_to_kernel_check((uint32_t)buf, sizeof(struct stat), 1);

	r = file_stat_fd(fd, buf);

	return SYSCALL_RETURN(r);
}
SYSCALL_END(fstat)

SYSCALL_DEFINE1(chdir, const char*, path) {
	path = (const char*)user_to_kernel_check((uint32_t)path, MAX_PATH_SIZE, 0);
	int r;
	struct dirent *dirent;

	r = dirent_find(path, -1, &dirent);
	if (r < 0)
		return SYSCALL_RETURN(r);

	task_curr->fs.pwd = dirent;
	return SYSCALL_RETURN(0);
}
SYSCALL_END(chdir)


SYSCALL_DEFINE2(getcwd, char*, buf, int, size) {
	buf = (char*)user_to_kernel_check((uint32_t)buf, size, 1);

	dirent_topath(task_curr->fs.pwd, buf, size);
	return SYSCALL_RETURN(0);
}
SYSCALL_END(getcwd)


SYSCALL_DEFINE2(mkdir, const char*, pathname, int, mode) {
	int r;
	pathname = (const char*)user_to_kernel_check((uint32_t)pathname, MAX_PATH_SIZE, 0);

	FileMode fmode;
	fmode.read = mode;
	fmode.write = mode >> 1;
	fmode.type = FileMode::FT_dir;

	r = create(pathname, fmode, -1);
	return SYSCALL_RETURN(r);
}
SYSCALL_END(mkdir)

SYSCALL_DEFINE3(mknod, const char*, pathname, int, mode, int, dev) {
	int r;
	pathname = (const char*)user_to_kernel_check((uint32_t)pathname, MAX_PATH_SIZE, 0);

	FileMode fmode;
	fmode.read = mode;
	fmode.write = mode >> 1;
	fmode.type = FileMode::FT_chrdev;

	r = create(pathname, fmode, dev);
	return SYSCALL_RETURN(r);
}
SYSCALL_END(mknod)


SYSCALL_DEFINE1(rmdir, const char*, path) {
	int r;
	path = (const char*)user_to_kernel_check((uint32_t)path, MAX_PATH_SIZE, 0);

	r = unlink(path, FileMode::FT_dir);
	return SYSCALL_RETURN(r);
}
SYSCALL_END(rmdir)


SYSCALL_DEFINE2(readdir, DIR*, dirp, struct dirent_user*, dirent) {
	dirp = (DIR*)user_to_kernel_check((uint32_t)dirp, sizeof(*dirp), 1);
	dirent = (struct dirent_user*)user_to_kernel_check((uint32_t)dirent, sizeof(*dirent), 1);
	int r;

	if (dirp->dd_fd == -1 || dirp->dd_fd > TASK_MAX_FILE_NR)
		return SYSCALL_RETURN(-1);

	// TODO: olmayan dosya girilirse hata ver
	struct file *f = task_curr->fs.files[dirp->dd_fd];
	if (f->fo->readdir == NULL)
		return SYSCALL_RETURN(-2);

	r = f->fo->readdir(f, dirent);
	return SYSCALL_RETURN(r);
}
SYSCALL_END(readdir)

SYSCALL_DEFINE3(lseek, int, fd, off_t, offset, int, whence) {
	int r;
	print_info(">> lseek %d %d %d\n", fd, offset, whence);

	r = file_lseek(fd, offset, whence);
	return SYSCALL_RETURN(r);
}
SYSCALL_END(lseek)


SYSCALL_DEFINE2(lstat, const char*, path, struct stat*, stat) {
	// FIXME: stat cagrisindan kopyalandi. duzelt
	int r;
	// FIXME: max length
	path = (char*)user_to_kernel_check((uint32_t)path, -1, 0);
	stat = (struct stat*)user_to_kernel_check((uint32_t)stat, sizeof(struct stat), 1);

	r = file_stat(path, stat);
	print_info("stat ok: %d\n", r);

	return SYSCALL_RETURN(r);
}
SYSCALL_END(lstat)
