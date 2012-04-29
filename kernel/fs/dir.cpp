#include <kernel/syscall.h>
#include "../task.h"
#include "../kernel.h"
#include "vfs.h"

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


SYSCALL_DEFINE0(readdir) {
	// unsigned int fd = get_param1(tf);
	// // FIXME: adres kontrolu
	// struct dirent *dirent = (struct dirent*)get_param2(tf);
	// unsigned int count = get_param3(tf);

	PANIC("readdir tamamlanmadi\n");

	return SYSCALL_RETURN(-1);
}
SYSCALL_END(readdir)
