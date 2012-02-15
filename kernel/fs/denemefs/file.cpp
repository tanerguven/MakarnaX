#include "../vfs.h"

#include <stdio.h>

#include "denemefs.h"

int denemefs_lookup(struct DirEntry *dir, struct DirEntry *r) {

	Deneme_inode *inode = inode_to_deneme(dir->inode);

	for (int i = 0 ; i < inode->subdentry.n ; i++) {
		if ( strcmp(inode->subdentry.d[i]->name, r->name) == 0) {
			r->inode = (struct inode*)inode->subdentry.d[i];
			return 0;
			break;
		}
	}

	return -1;
}

uint32_t denemefs_read(struct inode *inode, uint32_t offset, char *buf,
					   size_t size) {
	struct Deneme_inode *in = inode_to_deneme(inode);
	int i;

	const char *src = in->data + offset;

	for(i = 0 ; i < in->inode.size  && size > 0 ; i++, size--) {
		buf[i] = src[i];
	}

	return i;
}
