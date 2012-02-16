#include "../vfs.h"

#include <stdio.h>

#include "denemefs.h"

#include "../../kernel.h"

extern struct Deneme_inode di[];

int denemefs_lookup(struct DirEntry *dir, const char *name, uint32_t *no) {

	Deneme_inode *inode = inode_to_deneme(dir->inode);

	if (inode->ft != Deneme_inode::FT_DIR)
		return -1;

	Deneme_subdentry *sd = (Deneme_subdentry*)inode->data;
	for (int i = 0 ; i < sd->n ; i++) {
		if ( strcmp(sd->name[i], name) == 0) {
			*no = sd->no[i];
			return 0;
		}
	}

	return -1;
}

uint32_t denemefs_read(struct inode *inode, uint32_t offset, char *buf,
					   size_t size) {
	struct Deneme_inode *in = inode_to_deneme(inode);
	int i;

	if (in->ft != Deneme_inode::FT_FILE)
		return -1;

	const char *src = ((char*)in->data) + offset;
	for(i = 0 ; i < in->size  && size > 0 ; i++, size--) {
		buf[i] = src[i];
	}

	return i;
}
