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
#include <kernel/kernel.h>

#include "../task.h"
#include "fs.h"

int fs_task_init(Task *t, struct dirent *root) {
	t->fs.root = dirent_dup(root);
	t->fs.pwd = dirent_dup(root);
	return 0;
}

int fs_fork(Task *c) {
	c->fs.root = dirent_dup(task_curr->fs.root);
	c->fs.pwd = dirent_dup(task_curr->fs.pwd);

	/* dosya bilgilerini kopyala */
	for (int i = 0 ; i < TASK_MAX_FILE_NR ; i++) {
		if (task_curr->fs.files[i])
			c->fs.files[i] = file_dup(task_curr->fs.files[i]);
		else
			c->fs.files[i] = NULL;
	}

	return 0;
}

/* task_curr'in tum dosyalarini kapatir (task_free'de kullanim icin) */
void task_curr_free_files() {
	for (int i = 0 ; i < TASK_MAX_FILE_NR ; i++) {
		if (task_curr->fs.files[i]) {
			file_close(task_curr->fs.files[i]);
			task_curr->fs.files[i] = NULL;
		}
	}
}

struct file * task_curr_get_file(int fd) {
	ASSERT3(fd, <, TASK_MAX_FILE_NR);
	ASSERT3(fd, >=, 0);
	return task_curr->fs.files[fd];
}
