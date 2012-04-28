#include "denemefs.h"

#include "../../kernel.h"
#include "../../test_programs.h"

struct Deneme_inode di[100];

struct Deneme_inode* inode_to_deneme(struct inode *inode) {
	return &di[inode->ino];
}

/* RAM'de tanimli 2 dosyanin icerikleri */
static const char *dosya1 = "data 1";
static char dosya2[100] = "data 2";

static File_operations denemefs_file_op = {
	denemefs_read,
	denemefs_write,
	NULL,
	denemefs_open,
	denemefs_release
};

static File_operations denemefs_dir_op = {
	denemefs_read,
	NULL,
	NULL,
	denemefs_open,
	denemefs_release
};

static struct inode_operations denemefs_file_inode_op = {
	&denemefs_file_op,
    denemefs_lookup,
	denemefs_permission
};


static struct inode_operations denemefs_dir_inode_op = {
	&denemefs_dir_op,
	denemefs_lookup,
	denemefs_permission
};

/*
 * RAM uzerinde saklanan sahte bir dosya sistemi olusturuyoruz
 * di[0] -> root
 * dosya1 ve dosya2 isimli iki karakter dosyasi var
 * kernel monitordeki test programlari var
 *   (yield, hello, dongu vb.)
 */
void denemefs_init() {
	unsigned int i;
	int n_file = 5;

	memset(di, 0, sizeof(di));

	di[0].ft = Deneme_inode::FT_DIR;
	di[0].data = kmalloc(sizeof(struct Deneme_subdentry));
	di[0].size = sizeof(struct Deneme_subdentry);
	di[0].flags.rw = 0;

	di[1].ft = Deneme_inode::FT_FILE;
	di[1].data = (void*)dosya1;
	di[1].size = strlen("data 1")+1;
	di[1].flags.rw = 0;


	di[2].ft = Deneme_inode::FT_FILE;
	di[2].data = (void*)dosya2;
	di[2].size = 100;
	di[2].flags.rw = 1;


	di[3].ft = Deneme_inode::FT_DIR;
	di[3].data = kmalloc(sizeof(struct Deneme_subdentry));
	di[3].size = sizeof(struct Deneme_subdentry);
	di[3].flags.rw = 0;

	Deneme_subdentry *sd_3 = (Deneme_subdentry*)di[3].data;
	sd_3->n = 0;

	di[4].ft = Deneme_inode::FT_DIR;
	di[4].data = kmalloc(sizeof(struct Deneme_subdentry));
	di[4].size = sizeof(struct Deneme_subdentry);
	di[4].flags.rw = 0;

	Deneme_subdentry *sd_4 = (Deneme_subdentry*)di[4].data;
	sd_4->n = 0;
	/* sd_4'u sd_3 altina ekle */
	sd_3->n++;
	sd_3->no[0] = 4;
	strcpy(sd_3->name[0], "dir11");

	Deneme_subdentry *sd = (Deneme_subdentry*)di[0].data;
	sd->no[0] = 1;
	strcpy(sd->name[0], "dosya1");
	sd->no[1] = 2;
	strcpy(sd->name[1], "dosya2");
	sd->no[2] = 3;
	strcpy(sd->name[2], "dir1");
	sd->n = 3;

	/* /bin dizini */
	di[n_file].ft = Deneme_inode::FT_DIR;
	Deneme_subdentry *bindir = (Deneme_subdentry*)
		(di[n_file].data = kmalloc(sizeof(struct Deneme_subdentry)));
	di[n_file].size = sizeof(struct Deneme_subdentry);
	di[n_file].flags.rw = 0;

	sd->no[sd->n] = n_file;
	strcpy(sd->name[sd->n], "bin");
	n_file++;
	sd->n++;

	/* /bin dizini icerisindeki dosyalar */
	bindir->n = 0;
	/* kullanici programlarini dosya sistemine ekle */
	ASSERT(nr_test_programs < Deneme_subdentry_count);
	for (i = 0 ; i < nr_test_programs ; i++) {
		di[n_file].ft = Deneme_inode::FT_FILE;

		di[n_file].data = test_programs[i].addr;
		di[n_file].size = (uint32_t)test_programs[i].end -
			(uint32_t)test_programs[i].addr;
		di[n_file].flags.rw = 0;

		bindir->no[bindir->n] = n_file;
		strcpy(bindir->name[bindir->n], test_programs[i].name);
		n_file++;
		bindir->n++;
	}

	/* init_programs dosyasini ekle */
	di[n_file].ft = Deneme_inode::FT_FILE;
	di[n_file].data = &_binary_init_programs_start;
	di[n_file].size = (uint32_t)&_binary_init_programs_size;
	di[n_file].flags.rw = 0;

	sd->no[sd->n] = n_file;
	strcpy(sd->name[sd->n], "init_script");
	n_file++;
	sd->n++;

	printf(">> denemefs init OK\n");
	printf(">> denemefs: %d dir and %d file\n", 1, n_file-1);
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
		return -1;

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

uint32_t denemefs_read(struct File *f, char *buf, size_t size) {
	struct Deneme_inode *in = inode_to_deneme(f->inode);
	unsigned int i;

	if (in->ft != Deneme_inode::FT_FILE) {
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

	if (in->ft != Deneme_inode::FT_FILE)
		return -1;

	char *dest = ((char*)in->data) + f->fpos;
	for (i = 0 ; (i + f->fpos < in->size) && (size > 0) ; i++, size--) {
		dest[i] = buf[i];
	}

	return i;
}

int denemefs_open(struct File *f) {
	// printf("denemefs_open ino %d\n", f->inode->ino);
	return 0;
}

int denemefs_release(struct File *f) {
	// printf("denemefs_release\n");
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
