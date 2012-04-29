#include "denemefs.h"

#include <string.h>
#include "../../kernel.h"

static struct  File_operations denemefs_dir_op = {
	denemefs_read,
	NULL,
	NULL,
	denemefs_open,
	denemefs_release
};

struct inode_operations denemefs_dir_inode_op = {
	&denemefs_dir_op,
	denemefs_lookup,
	denemefs_create,
	denemefs_unlink,
	denemefs_mkdir,
	denemefs_rmdir,
	denemefs_mknod,
	denemefs_permission
};

// FIXME: bos yer bulmak icin sd->no[i] == 0 olan bulunmali
int denemefs_add_entry(struct inode *dir, const char *name, struct inode *i) {
	struct inode inode;
	int r = denemefs_lookup(dir, name, &inode);
	if (r == 0)
		return -1;

	Deneme_subdentry *sd = (Deneme_subdentry*)inode_to_deneme(dir)->data;
	sd->no[sd->n] = i->ino;
	strcpy(sd->name[sd->n], name);
	sd->n++;
	return 0;
}

int denemefs_remove_entry(struct inode *dir, const char *name) {
	int i;
	Deneme_subdentry *sd = (Deneme_subdentry*)inode_to_deneme(dir)->data;

	for (i = 0 ; i < sd->n ; i++) {
		if (strcmp(sd->name[i], name) == 0) {
			sd->no[i] = 0;
			strcpy(sd->name[i], "");
			return 0;
		}
	}
	return -1;
}

int denemefs_mkdir(struct inode *i_dir, const char *name, int mode) {
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
	inode_new->data = kmalloc(sizeof(struct Deneme_subdentry));
	inode_new->size = sizeof(struct Deneme_subdentry);
	inode_new->mode.read = (mode & 1);
	inode_new->mode.write = (mode & 2) >> 1;
	memset(inode_new->data, 0, inode_new->size);
	inode_new->mode.type = FileMode::FT_dir;

	return 0;
}

static int is_empty_dir(struct inode* inode) {
	// TODO: --
	return 1;
}

int denemefs_rmdir(struct inode *i_dir, const char *name) {
	int r;
	struct inode inode;

	r = denemefs_lookup(i_dir, name, &inode);
	if (r < 0)
		return -1; // not exist

	if (! is_empty_dir(&inode))
		return -1; // not empty

	r = denemefs_remove_entry(i_dir, name);
	ASSERT(r == 0); // lookup ile bulunabiliyorsa, olmak zorunda

	denemefs_free_inode(&inode);

	return 0;
}
