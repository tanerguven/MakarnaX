#include "denemefs.h"

#include "../../kernel.h"
#include "../../test_programs.h"

struct Deneme_inode di[100];

struct Deneme_inode* inode_to_deneme(struct inode *inode) {
	return &di[inode->ino];
}

/* RAM'de tanimli 2 dosyanin icerikleri */
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
	di[0].data = kmalloc(sizeof(struct Deneme_subdentry));

	di[1].ft = Deneme_inode::FT_FILE;
	di[1].data = (void*)dosya1;
	di[1].size = strlen("data 1")+1;

	di[2].ft = Deneme_inode::FT_FILE;
	di[2].data = (void*)dosya2;
	di[2].size = strlen("data 2")+1;

	Deneme_subdentry *sd = (Deneme_subdentry*)di[0].data;
	sd->no[0] = 1;
	strcpy(sd->name[0], "dosya1");
	sd->no[1] = 2;
	strcpy(sd->name[1], "dosya2");
	sd->n = 2;

	for (i = 0 ; i < nr_user_programs ; i++) {
		di[i+3].ft = Deneme_inode::FT_FILE;

		di[i+3].data = user_programs[i].addr;
		di[i+3].size = (uint32_t)user_programs[i].end -
			(uint32_t)user_programs[i].addr;
		sd->no[sd->n] = i+3;
		strcpy(sd->name[sd->n], user_programs[i].name);

		sd->n++;
	}

	printf(">> denemefs init OK\n");
	printf(">> denemefs: %d dir and %d file\n", 0, sd->n);
}

static struct inode_operations denemefs_inode_op = {
	denemefs_lookup,
};

int denemefs_read_super(struct SuperBlock* sb) {
	ASSERT(sb->dev == 123);
	ASSERT(sb->fs_type == 123);

	sb->root = (struct DirEntry*)kmalloc(sizeof(DirEntry*));
	sb->root->init();
	sb->root->mounted = 1;
	strcpy(sb->root->name, "");

	sb->root->inode = (struct inode*)kmalloc(sizeof(struct inode));
	sb->root->inode->init(0, sb, &denemefs_inode_op);

	return 0;
}
