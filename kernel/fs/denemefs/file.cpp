
#include <kernel/kernel.h>
#include <string.h>

#include "denemefs.h"

static struct File_operations denemefs_file_op = {
	denemefs_read,
	denemefs_write,
	NULL,
	denemefs_open,
	denemefs_release
};

struct inode_operations denemefs_file_inode_op = {
	&denemefs_file_op,
    denemefs_lookup,
	NULL, /* create */
	NULL, /* unlink */
	NULL, /* mkdir */
	NULL, /* rmdir */
	NULL, /* mknod */
	denemefs_permission
};


uint32_t denemefs_read(struct File *f, char *buf, size_t size) {
	struct Deneme_inode *in = inode_to_deneme(f->inode);
	unsigned int i;

	if (in->mode.type != FileMode::FT_regular) {
		// PANIC("tanimsiz durum");
		return -1;
	}

	const char *src = ((char*)in->data) + f->fpos;
	for(i = 0 ; (i + f->fpos < in->size) && (size > 0) ; i++, size--) {
		buf[i] = src[i];
	}

	return i;
}

uint32_t denemefs_write(struct File *f, const char *buf, size_t size) {
	struct Deneme_inode *in = inode_to_deneme(f->inode);
	unsigned int i;

	if (in->mode.type != FileMode::FT_regular)
		return -1;

	char *dest = ((char*)in->data) + f->fpos;
	for (i = 0 ; (i + f->fpos < in->size) && (size > 0) ; i++, size--) {
		dest[i] = buf[i];
	}

	return i;
}

int denemefs_open(struct File *f) {
	// print_info("denemefs_open ino %d\n", f->inode->ino);
	return 0;
}

int denemefs_release(struct File *f) {
	// print_info("denemefs_release\n");
	return 0;
}

int denemefs_create(struct inode *i_dir, const char* name, int mode, struct inode *i_dest) {
	struct inode i;
	int r;
	Deneme_inode *inode_new;

	r = denemefs_new_inode(i_dir, &i);
	if (r < 0)
		return -1; /* diskte inode kalmadi */

	r = denemefs_add_entry(i_dir, name, &i);
	if (r < 0) {
		denemefs_free_inode(&i);
		/* i exist or i_dir not dir */
		return -1;
	}

	inode_new = inode_to_deneme(&i);
	inode_new->data = kmalloc(1000);
	inode_new->size = 1000;
	inode_new->mode.read = (mode & 1);
	inode_new->mode.write = (mode & 2) >> 1;
	memset(inode_new->data, 0, inode_new->size);
	inode_new->mode.type = FileMode::FT_regular;

	r = denemefs_lookup(i_dir, name, i_dest);
	ASSERT(r == 0); /* dosyayi yeni olusturduk, dosya bulunmali */

	return 0;
}

int denemefs_unlink(struct inode *i_dir, const char* name) {
	int r;
	struct inode inode;

	r = denemefs_lookup(i_dir, name, &inode);
	if (r < 0)
		return -1; // not exist

	r = denemefs_remove_entry(i_dir, name);
	ASSERT(r == 0); // lookup ile bulunabiliyorsa, olmak zorunda

	denemefs_free_inode(&inode);

	return 0;
}

int denemefs_mknod(struct inode *i_dir, const char *name, FileMode mode, int dev) {
	struct inode i;
	int r;
	Deneme_inode *inode_new;

	r = denemefs_new_inode(i_dir, &i);
	if (r < 0)
		return -1; /* diskte inode kalmadi */

	r = denemefs_add_entry(i_dir, name, &i);
	if (r < 0) {
		denemefs_free_inode(&i);
		/* i exist or i_dir not dir */
		return -1;
	}

	inode_new = inode_to_deneme(&i);
	inode_new->mode = mode;
	inode_new->dev = dev;
	inode_new->data = NULL;
	inode_new->size = 0;

	return 0;
}
