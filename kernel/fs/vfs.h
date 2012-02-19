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
	struct inode_operations *op;

	inline void init(uint32_t ino, SuperBlock *sb, struct inode_operations *op,
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
	char name[256];

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
	char path[1536];
	struct File_operations *fo;
	struct inode *inode;
	int fpos;
};

struct File_operations {
	uint32_t (*read)(struct File *f, char *buf, size_t size);
	int (*readdir)(struct File *f, struct dirent *dirent, uint32_t count);
	int (*open)(struct File *f);
	int (*release)(struct File *f);
};

struct inode_operations {

	struct File_operations *default_file_ops;

	/* dir icerisinde name isimli dosyayi arar, dest ile dondurur */
	int (*lookup)(struct inode *i_dir, const char* name, struct inode *i_dest);
};


extern int lookup(struct DirEntry *dir, const char *fn, struct DirEntry **dentry);
extern int find_dir_entry(const char *path, struct DirEntry **dest);
extern int dir_entry_to_path(struct DirEntry *dirent, char *buf, size_t size);

#endif /* _VFS_H_ */
