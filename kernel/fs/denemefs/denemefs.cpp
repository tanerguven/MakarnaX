
#include <kernel/kernel.h>
#include <string.h>

#include "denemefs.h"

struct Deneme_inode di[100];

struct Deneme_inode* inode_to_deneme(struct inode *inode) {
	return &di[inode->ino];
}

#include <tar.h>
extern uint32_t initrd_start, initrd_end;

void mount_initrd(struct inode *root) {
	int i;
	int r;
	uint32_t address = initrd_start;

	for (i = 0; ; i++) {
		struct inode inode;
		struct tar_header *header = (struct tar_header *)address;
		const char *name = strrchr(header->name, '/')+1;

		print_info("%s %c\n", header->name, header->typeflag[0]);

		if (header->name[0] == '\0')
			break;
		unsigned long size = tar_getsize(header->size);

		if (header->typeflag[0] == '0') {
			r = denemefs_create(root, name, 1, &inode);
			ASSERT(r == 0);
			di[inode.ino].data = (void*)(address+512);
			di[inode.ino].size = size;
		}

		address += ((size / 512) + 1) * 512;
		if (size % 512)
			address += 512;
	}
}

/*
 * RAM uzerinde saklanan sahte bir dosya sistemi olusturuyoruz
 */
void denemefs_init() {
	unsigned int i;
	struct inode root_inode, dir1_inode, bin_inode, newinode, initrd_inode;
	int r;
	Deneme_inode *in;

	memset(di, 0, sizeof(di));

	/* / (root) */
	di[0].nlinks = 1;
	di[0].mode.type = FileMode::FT_dir;
	di[0].mode.read = 1;
	di[0].data = kmalloc(sizeof(struct Deneme_subdentry));
	di[0].size = sizeof(struct Deneme_subdentry);
	Deneme_subdentry *root_sd = (Deneme_subdentry*)di[0].data;
	root_sd->n = 0;
	root_inode.ino = 0;

	/* /home */
	r = denemefs_mkdir(&root_inode, "home", 3);
	ASSERT(r == 0);
	r = denemefs_lookup(&root_inode, "home", &dir1_inode);
	ASSERT(r == 0);

	/* /dosya1 */
	r = denemefs_create(&root_inode, "dosya1", 1, &newinode);
	ASSERT(r == 0);
	strcpy((char*)di[newinode.ino].data, "d1 icerik");

	/* /dosya2 */
	r = denemefs_create(&root_inode, "dosya2", 3, &newinode);
	ASSERT(r == 0);
	strcpy((char*)di[newinode.ino].data, "data2");

	/* /dir1 */
	r = denemefs_mkdir(&root_inode, "dir1", 3);
	ASSERT(r == 0);
	r = denemefs_lookup(&root_inode, "dir1", &dir1_inode);
	ASSERT(r == 0);

	/* /dir1/dir11 */
	r = denemefs_mkdir(&dir1_inode, "dir11", 3);
	ASSERT(r == 0);

	/* /initrd */
	if (initrd_start) {
		r = denemefs_mkdir(&root_inode, "initrd", 1);
		ASSERT(r == 0);
		r = denemefs_lookup(&root_inode, "initrd", &initrd_inode);
		ASSERT(r == 0);
		mount_initrd(&initrd_inode);
	}

	print_info(">> denemefs init OK\n");
}

int denemefs_read_super(struct SuperBlock* sb) {
	ASSERT(sb->dev == 123);
	ASSERT(sb->fs_type == 123);

	sb->root = dirent_alloc();
	sb->root->mounted = 1;
	strcpy(sb->root->name, "");

	sb->root->inode = inode_alloc();
	struct inode *i = sb->root->inode;
	i->ino = 0;
	i->superblock = sb;
	i->op = &denemefs_dir_inode_op;
	i->size = di[0].size;

	return 0;
}

int denemefs_lookup(struct inode* i_dir, const char *name, struct inode *i_dest) {

	Deneme_inode *dir = inode_to_deneme(i_dir);

	if (dir->mode.type != FileMode::FT_dir)
		return -2;

	Deneme_subdentry *sd = (Deneme_subdentry*)dir->data;
	for (int i = 0 ; i < sd->n ; i++) {
		if ( strcmp(sd->name[i], name) == 0) {
			Deneme_inode *inode = &di[sd->no[i]];
			ASSERT(inode->nlinks > 0);

			switch(inode->mode.type) {
			case FileMode::FT_dir:
				i_dest->op = &denemefs_dir_inode_op;
				break;
			case FileMode::FT_regular:
				i_dest->op = &denemefs_file_inode_op;
				break;
			// case FileMode::FT_chrdev:
			// 	i_dest->dev = di[sd->no[i]].dev;
			// 	i_dest->op = &chrdev_inode_operations;
			// 	break;
			default:
				print_error("file type: %d\n", di[sd->no[i]].mode.type);
				PANIC("Unknown file type");
			}
			i_dest->ino = sd->no[i];
			i_dest->superblock = i_dir->superblock;
			i_dest->size = di[sd->no[i]].size;
			i_dest->mode = di[sd->no[i]].mode;
			return 0;
		}
	}

	return -1;
}

void denemefs_free_inode(struct inode *i) {
	if (di[i->ino].data)
		kfree(di[i->ino].data);
	memset(&di[i->ino], 0, sizeof(di[i->ino]));
	di[i->ino].nlinks = 0;
}

int denemefs_new_inode(struct inode *i_dir, struct inode *dest) {
	int ino = find_empty_di();
	if (ino < 0)
		return -1; /* diskte inode kalmadi */

	/* inode olustur */
	di[ino].nlinks = 1;
	dest->ino = ino;

	return 0;
}

int denemefs_permission(struct inode *i, int flags) {
	struct Deneme_inode *in = inode_to_deneme(i);
	/* dosya rw ise tum erisim tiplerine izin ver */
	if (in->mode.write & in->mode.read)
		return 1;
	/* readonly erisime her zaman izin ver (denemefs'de writeonly dosya yok) */
	if ((flags & 0x3) == 1)
		return 1;
	return 0;
}


int find_empty_di() {
	int i;
	//FIXME: assert insterrupts disable
	for (i = 0 ; i < 100 ; i++) {
		if (di[i].nlinks == 0) {
			di[i].nlinks = 1; // baska processler bos gormesin
			return i;
		}
	}
	return -1;
}
