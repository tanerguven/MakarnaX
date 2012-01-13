/*
 * Copyright (C) 2011 Taner Guven <tanerguven@gmail.com>
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

#ifndef _IPC_H_
#define _IPC_H_

#include <wmc/list.h>
#include <wmc/idhashtable.h>

#include "../task.h"

struct LI_SharedMem {
	static const ptr_t offset_node_value;
};
typedef List_2<struct SharedMemDesc, LI_SharedMem> SharedMemList_t;

struct LI_SharedMem_Task {
	static const ptr_t offset_node_value;
};
typedef List_2<struct SharedMemDesc, LI_SharedMem_Task> SharedMem_Task_List_t;

struct SharedMemDesc {
	uint32_t start; // user address
	uint32_t end;
	SharedMemList_t::node_t list_node;
	SharedMem_Task_List_t::node_t info_task_list_node;
	struct SharedMemInfo* info;

	inline struct Task* task();

	inline void init() {
		list_node.init();
		info_task_list_node.init();
	}
};

extern int ipc_fork(Task *child);
extern void ipc_task_free(Task *t);
extern uint32_t mem_ipc();

#endif /* _IPC_H_ */
