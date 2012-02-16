#include "denemefs.h"

#include "../../kernel.h"
#include "../../test_programs.h"

struct Deneme_inode di[100];

static const char *dosya1 = "data 1";
static const char *dosya2 = "data 2";

/*
 * RAM uzerinde saklanan sahte bir dosya sistemi olusturuyoruz
 * di[0] -> root
 * dosya1 ve dosya2 isimli iki karakter dosyasi var
 * kernel monitordeki test programlari var
 *   (yield, hello, dongu vb.)
 */
void denemefs_init() {
	int i;

	di[0].ft = Deneme_inode::FT_DIR;
	strcpy(di[0].name, "dizin0");
	di[0].data = kmalloc(sizeof(struct Deneme_subdentry));

	di[1].ft = Deneme_inode::FT_FILE;
	strcpy(di[1].name, "dosya1");
	di[1].data = (void*)dosya1;
	di[1].inode.size = strlen("data 1")+1;

	di[2].ft = Deneme_inode::FT_FILE;
	strcpy(di[2].name, "dosya2");
	di[2].data = (void*)dosya2;
	di[2].inode.size = strlen("data 2")+1;

	Deneme_subdentry *sd = (Deneme_subdentry*)di[0].data;
	sd->d[0] = &di[1];
	sd->d[1] = &di[2];
	sd->n = 2;

	for (i = 0 ; i < nr_user_programs ; i++) {
		di[i+3].ft = Deneme_inode::FT_FILE;
		strcpy(di[i+3].name, user_programs[i].name);
		di[i+3].data = user_programs[i].addr;
		di[i+3].inode.size = (uint32_t)user_programs[i].end -
			(uint32_t)user_programs[i].addr;
		sd->d[sd->n] = &di[i+3];
		sd->n++;
	}

	printf(">> denemefs init OK\n");
	printf(">> denemefs: %d dir and %d file\n", 0, sd->n);
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
