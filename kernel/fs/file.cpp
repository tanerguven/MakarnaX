#include "../task.h"
#include "../kernel.h"
#include "vfs.h"

extern uint32_t denemefs_read(struct inode *, uint32_t, char *, size_t);

uint32_t fo_charfile_read(struct File *f, char *buf, size_t size) {

	uint32_t r = denemefs_read(f->inode, f->fpos, buf, size);

	f->fpos += r;

	return r;
}

static File_operations fo_charfile = {
	fo_charfile_read,
	NULL,
	NULL,
	NULL
};

asmlink void sys_open() {
	Trapframe *tf = task_curr->registers();
	const char *filename = (const char*)user_to_kernel(get_param1(tf), 256, 0);
	// int flags = (int)get_param2(tf);
	// int mode = (int)get_param3(tf);
	DirEntry *file_dentry;
	int fd;

	uint32_t eflags = eflags_read();
	cli();

	lookup(task_curr->pwd, filename, &file_dentry);
	if (file_dentry == NULL)
	  return set_return(tf, -1); // FIXME: dosya yok

	File *f = (File*)kmalloc(sizeof(File));
	strcpy(f->path, "/");
	strcpy(&f->path[1], filename);
	f->fo = &fo_charfile;
	f->inode = file_dentry->inode;
	f->fpos = 0;

	for (fd = 0 ; fd < TASK_MAX_FILE_NR ; fd++) {
		if (task_curr->files[fd] == NULL) {
			task_curr->files[fd] = f;
			// FIXME: --
			f->inode->ref_count++;
			break;
		}
	}

	eflags_load(eflags);
	return set_return(tf, fd);
}

void do_close(int fd) {
	ASSERT(task_curr->files[fd] != NULL);
	ASSERT(task_curr->files[fd]->inode->ref_count > 0);

	// FIXME: --
	task_curr->files[fd]->inode->ref_count--;

	kfree(task_curr->files[fd]);
	task_curr->files[fd] = NULL;
}

asmlink void sys_close() {
	Trapframe *tf = task_curr->registers();
	unsigned int fd = get_param1(tf);

	uint32_t eflags = eflags_read();
	cli();

	if (task_curr->files[fd] == NULL)
		return set_return(tf, -1);

	do_close(fd);

	eflags_load(eflags);
	return set_return(tf, 0);
}

asmlink void sys_read() {
	Trapframe *tf = task_curr->registers();
	unsigned int fd = get_param1(tf);
	char *buf = (char*)user_to_kernel(get_param2(tf), 256, 1);
	unsigned int count = get_param3(tf);
	size_t r;

	r = task_curr->files[fd]->fo->read(task_curr->files[fd], buf, count);

	return set_return(tf, r);
}

asmlink void sys_readdir() {
	Trapframe *tf = task_curr->registers();
	// unsigned int fd = get_param1(tf);
	// // FIXME: adres kontrolu
	// struct dirent *dirent = (struct dirent*)get_param2(tf);
	// unsigned int count = get_param3(tf);

	return set_return(tf, -1);
}

void task_curr_free_files() {
	for (int i = 0 ; i < TASK_MAX_FILE_NR ; i++) {
		if (task_curr->files[i])
			do_close(i);
	}
}
