/*
 * Copyright (C) 2012 Taner Guven <tanerguven@gmail.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef _VFS_H_
#define _VFS_H_

#include <types.h>
#include <wmc/list.h>

#define MAX_DIR_ENTRY_SIZE 256
#define MAX_PATH_SIZE 1536

struct FileMode {
	uint32_t read: 1;
	uint32_t write: 1;
	uint32_t __: 6;
	uint32_t type: 8;

	enum {
		FT_regular = 1,
		FT_dir = 2,
		FT_symlink = 3,
		FT_chrdev = 4,
		FT_blkdev = 5,
		FT_fifo = 6,
	};

	inline uint16_t get_mode() {
		return (*(uint16_t*)this) & 0xFF;
	}

} __attribute__((packed));


struct SuperBlock {
	int dev;
	int fs_type;
	struct DirEntry *root;
};

struct inode {
	uint32_t ino;
	SuperBlock *superblock;
	int ref_count;
	uint32_t size;
	const struct inode_operations *op;
	int dev;
	FileMode mode;

	// TODO: file lock

	inline void init(uint32_t ino, SuperBlock *sb, const struct inode_operations *op,
					 uint32_t size) {
		this->ino = ino;
		this->superblock = sb;
		this->ref_count = 0;
		this->op = op;
		this->size = size;
	}
};

/**
 * directory entry, dizin ya da dosya
 */
struct DirEntry {
	define_list(DirEntry, Subdirs);
	Subdirs::node_t node_subdirs;

	int mounted;
	struct inode *inode;
	char name[MAX_DIR_ENTRY_SIZE];

	struct DirEntry * parent;
	Subdirs subdirs;

	inline void init() {
		mounted = 0;
		parent = NULL;
		node_subdirs.init();
		subdirs.init();
	}

	inline void add_subdir(struct DirEntry *dir) {
		subdirs.push_back(&dir->node_subdirs);
		dir->parent = this;
	}
};


struct File {
	char path[MAX_PATH_SIZE];
	const struct File_operations *fo;
	struct inode *inode;
	int fpos;
	int flags;
};

struct File_operations {
	/** @return: okunan byte sayisi, -1: hata (?) */
	uint32_t (*read)(struct File *f, char *buf, size_t size);
	/** @return: yazilan byte sayisi, -1: hata (?) */
	uint32_t (*write)(struct File* f, const char *buf, size_t size);
	int (*readdir)(struct File *f, struct dirent *dirent, uint32_t count);
	int (*open)(struct File *f);
	int (*release)(struct File *f);
};

struct inode_operations {

	const struct File_operations *default_file_ops;

	/* dir icerisinde name isimli dosyayi arar, dest ile dondurur */
	int (*lookup)(struct inode *i_dir, const char* name, struct inode *i_dest);
	int (*create)(struct inode *i_dir, const char* name, int mode, struct inode *i_dest);
	int (*unlink)(struct inode *i_dir, const char* name);
	int (*mkdir)(struct inode *i_dir, const char *name, int mode);
	int (*rmdir)(struct inode *i_dir, const char *name);
	int (*mknod)(struct inode *i_dir, const char *name, FileMode mode, int dev);
	int (*permission)(struct inode* i, int flags);
};


extern int lookup(struct DirEntry *dir, const char *fn, struct DirEntry **dentry);
extern int find_dir_entry(const char *path, size_t len, struct DirEntry **dest);
extern int dir_entry_to_path(struct DirEntry *dirent, char *buf, size_t size);
extern int find_file_and_dir(const char* path, DirEntry **dentry, const char **name);
extern void remove_from_dir_entry_cache(const char* path);
extern int permission(struct inode* inode, int flags);

extern int do_open(File **f, const char *path, int flags);
extern void do_close(File *f);
extern int do_read(File *f, char *buf, unsigned int count);


extern const inode_operations chrdev_inode_operations;

#endif /* _VFS_H_ */
