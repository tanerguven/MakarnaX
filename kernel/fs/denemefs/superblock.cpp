#include "denemefs.h"

#include "../../kernel.h"

struct Deneme_inode di[100];

void denemefs_init() {
	di[0].ft = Deneme_inode::FT_DIR;
	strcpy(di[0].name, "dizin0");

	di[1].ft = Deneme_inode::FT_FILE;
	strcpy(di[1].name, "dosya1");
	strcpy(di[1].data, "data 1");
	di[1].inode.size = strlen("data 1")+1;

	di[2].ft = Deneme_inode::FT_FILE;
	strcpy(di[2].name, "dosya2");
	strcpy(di[2].data, "data 2");
	di[2].inode.size = strlen("data 2")+1;

	di[0].subdentry.d[0] = &di[1];
	di[0].subdentry.d[1] = &di[2];
	di[0].subdentry.n = 2;

	printf(">> denemefs init OK\n");
}

int denemefs_read_super(struct SuperBlock* sb) {
	ASSERT(sb->dev == 123);
	ASSERT(sb->fs_type == 123);

	sb->root = (struct DirEntry*)kmalloc(sizeof(DirEntry*));
	sb->root->init();
	sb->root->inode = deneme_to_inode(&di[0]);
	sb->root->mounted = 1;
	strcpy(sb->root->name, "");

	sb->root->inode->ino = 0;
	sb->root->inode->op = (struct inode_operations*)
		kmalloc(sizeof(struct inode_operations));
	sb->root->inode->op->lookup = denemefs_lookup;
	sb->root->inode->superblock = sb;
	sb->root->inode->size = 0;

	return 0;
}
