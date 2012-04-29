#include "denemefs.h"

#include "../../kernel.h"
#include "../../test_programs.h"

struct Deneme_inode di[100];

struct Deneme_inode* inode_to_deneme(struct inode *inode) {
	return &di[inode->ino];
}

/*
 * RAM uzerinde saklanan sahte bir dosya sistemi olusturuyoruz
 */
void denemefs_init() {
	unsigned int i;
	struct inode root_inode, dir1_inode, bin_inode, newinode;
	int r;
	Deneme_inode *in;

	memset(di, 0, sizeof(di));
	for (i = 0 ; i < 100 ; i++)
		di[i].ft = Deneme_inode::FT_NULL;

	/* / (root) */
	di[0].ft = Deneme_inode::FT_DIR;
	di[0].data = kmalloc(sizeof(struct Deneme_subdentry));
	di[0].size = sizeof(struct Deneme_subdentry);
	di[0].flags.rw = 0;
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

	/* /bin */
	r = denemefs_mkdir(&root_inode, "bin", 1);
	ASSERT(r == 0);
	r = denemefs_lookup(&root_inode, "bin", &bin_inode);
	ASSERT(r == 0);

	/* /bin/ * */
	ASSERT(nr_test_programs < Deneme_subdentry_count);
	for (i = 0 ; i < nr_test_programs ; i++) {
		r = denemefs_create(&bin_inode, test_programs[i].name, 1, &newinode);
		ASSERT(r == 0);
		/* kernel'e gomulu dosyalari map et */
		in = inode_to_deneme(&newinode);
		kfree(in->data);
		in->data = test_programs[i].addr;
		in->size = (uint32_t)test_programs[i].end - (uint32_t)test_programs[i].addr;
	}

	/* /init_script */
	r = denemefs_create(&root_inode, "init_script", 1, &newinode);
	ASSERT(r == 0);
	/* kernel gomulu init_programs dosyasini map et */
	in = inode_to_deneme(&newinode);
	kfree(in->data);
	in->data = &_binary_init_programs_start;
	in->size = (uint32_t)&_binary_init_programs_size;

	printf(">> denemefs init OK\n");
}

int denemefs_read_super(struct SuperBlock* sb) {
	ASSERT(sb->dev == 123);
	ASSERT(sb->fs_type == 123);

	sb->root = (struct DirEntry*)kmalloc(sizeof(DirEntry));
	sb->root->init();
	sb->root->mounted = 1;
	strcpy(sb->root->name, "");

	sb->root->inode = (struct inode*)kmalloc(sizeof(struct inode));
	sb->root->inode->init(0, sb, &denemefs_dir_inode_op, di[0].size);

	return 0;
}

int denemefs_lookup(struct inode* i_dir, const char *name, struct inode *i_dest) {

	Deneme_inode *inode = inode_to_deneme(i_dir);

	if (inode->ft != Deneme_inode::FT_DIR)
		return -2;

	Deneme_subdentry *sd = (Deneme_subdentry*)inode->data;
	for (int i = 0 ; i < sd->n ; i++) {
		if ( strcmp(sd->name[i], name) == 0) {
			/* dosya ise file_op, dizin ise dir_op */
			const inode_operations *iop = &denemefs_file_inode_op;
			if (di[sd->no[i]].ft == Deneme_inode::FT_DIR)
				iop = &denemefs_dir_inode_op;

			i_dest->init(sd->no[i], i_dir->superblock, iop, di[sd->no[i]].size);
			return 0;
		}
	}

	return -1;
}

void denemefs_free_inode(struct inode *i) {
	if (di[i->ino].data)
		kfree(di[i->ino].data);
	memset(&di[i->ino], 0, sizeof(di[i->ino]));
	di[i->ino].ft = Deneme_inode::FT_NULL;
}

int denemefs_new_inode(struct inode *i_dir, struct inode *dest) {
	int ino = find_empty_di();
	if (ino < 0)
		return -1; /* diskte inode kalmadi */

	/* inode olustur */
	di[ino].ft = Deneme_inode::FT_UNUSED;
	dest->ino = ino;

	return 0;
}

int denemefs_permission(struct inode *i, int flags) {
	struct Deneme_inode *in = inode_to_deneme(i);
	/* dosya rw ise tum erisim tiplerine izin ver */
	if (in->flags.rw)
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
		if (di[i].ft == Deneme_inode::FT_NULL) {
			di[i].ft = Deneme_inode::FT_UNUSED; // baska processler bos gormesin
			return i;
		}
	}
	return -1;
}
