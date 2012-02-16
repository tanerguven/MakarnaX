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

#include "vfs.h"

#include <string.h>
#include "../kernel.h"
#include "../task.h"

#include "denemefs/denemefs.h"

#define NR_SUPERBLOCKS 32

static struct SuperBlock superblocks[NR_SUPERBLOCKS];

void mount_root(Task *init_task) {

	memset(superblocks, 0, sizeof(superblocks));
	superblocks[0].dev = 123;
	superblocks[0].fs_type = 123;

	denemefs_read_super(&superblocks[0]);

	init_task->root = init_task->pwd = superblocks[0].root;

	printf(">> mount_root OK\n");
}
