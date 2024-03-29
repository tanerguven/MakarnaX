
#include <kernel/kernel.h>
#include <string.h>

#include "fs.h"

#define MAX_CHRDEV 64

struct Device {
	const char *name;
	const file_operations *file_op;
};

static struct Device chrdevs[MAX_CHRDEV] = {
  { NULL, NULL }
};

int register_chrdev(uint32_t chrdev_no, const char* name, const file_operations *file_op) {
	if (chrdev_no >= MAX_CHRDEV)
		return -1; // hatali device no
	if (chrdevs[chrdev_no].file_op)
		return -2; // kullanimda
	chrdevs[chrdev_no].name = name;
	chrdevs[chrdev_no].file_op = file_op;
	return 0;
}

int unregister_chrdev(int chrdev_no, const char *name) {
	if (chrdev_no >= MAX_CHRDEV)
		return -1; // hatali device no
	if (!chrdevs[chrdev_no].file_op)
		return -2; // kullanilmiyor
	if (strcmp(chrdevs[chrdev_no].name, name) != 0)
		return -3; // chrdev_no ile name uyusmuyor
	chrdevs[chrdev_no].name = NULL;
	chrdevs[chrdev_no].file_op = NULL;
	return 0;
}

int chrdev_open(struct file *fp) {
	int dev = fp->inode->dev;
	if (dev >= MAX_CHRDEV)
		return -1;
	if (!chrdevs[dev].file_op)
		return -1;
	fp->fo = chrdevs[dev].file_op;

	return 0;
}

static const file_operations chrdev_file_op = {
	NULL, /* read */
	NULL, /* write */
	NULL, /* readdir */
	chrdev_open, /* open */
	NULL, /* release */
};

const inode_operations chrdev_inode_operations = {
	&chrdev_file_op,
	NULL, /* lookup */
	NULL, /* create */
	NULL, /* unlink */
	NULL, /* mkdir */
	NULL, /* rmdir */
	NULL, /* mknod */
	NULL, /* permission */
};


uint32_t stdio_read(struct file *f, char *buf, size_t size) {

	return 0;
}

uint32_t stdio_write(struct file* f, const char *buf, size_t size) {
	// FIXME: --
	__kernel_print("%s", buf);
	return size;
}

const file_operations stdout_operations = {
	NULL, /* read */
	stdio_write,
	NULL, /* readdir */
	chrdev_open, /* open */
	NULL, /* release */
};

const file_operations stdin_operations = {
	stdio_read, /* read */
	NULL, /* write */
	NULL, /* readdir */
	chrdev_open, /* open */
	NULL, /* release */
};

void init_devices() {
	memset(chrdevs, 0, sizeof(chrdevs));

	register_chrdev(10, "stdin", &stdin_operations);
	register_chrdev(11, "stdout", &stdout_operations);
}
