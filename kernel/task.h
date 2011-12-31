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

/*
 * Task (process) ile ilgili veriyapilari.
 */

#ifndef TASK_H_
#define TASK_H_

#include <types.h>
#include <wmc/list.h>
#include <wmc/idhashtable.h>
#include "panic.h"

#include "trap.h"
#include "memory/virtual.h"

#include "ipc/ipc.h"

#define DEFAULT_PRIORITY 3

// TODO: bunlari makro olarak tanimlarsak guzel olabilir
struct LI_Alarm {
	static const  ptr_t offset_node_value;
};
typedef List_2<struct Task, LI_Alarm> AlarmList_t;

struct LI_Child {
	static const ptr_t offset_node_value;
};
typedef List_2<struct Task, LI_Child> ChildList_t;

struct LI_Task {
	static const ptr_t offset_node_value;
};
typedef List_2<struct Task, LI_Task> TaskList_t;

struct HI_TaskId {
	static const ptr_t offset_struct;
	static const ptr_t offset_id;
};
typedef IdHashTable<struct Task, HI_TaskId> TaskIdHashTable_t;



// FIXME: bunlari uygun yerlere tasi
struct SignalAction {
	uint32_t handler;
};

struct SignalInfo {
	uint32_t sig;
	uint32_t pending;
	SignalAction action[32];
	inline void init() {
		sig = 0;
		pending = 0;
	}
};

inline Trapframe* current_registers() {
	return (Trapframe*)(MMAP_KERNEL_STACK_TOP - sizeof(Trapframe));
}

extern struct Task* task_curr;

struct Task {
	enum  State {
		State_not_runnable = -1,
		State_running = 0,
		State_interruptible = 1,
		State_uninterruptible = 2,
		State_zombie = 3,
		State_stopped = 4
	};

	TaskList_t::node_t list_node;
	ChildList_t::node_t childlist_node;
	TaskIdHashTable_t::node_t id_hash_node;

	/* Trapframe registers; */
	Trapframe registers_user;
	Trapframe registers_signal;
	Trapframe registers_kernel;

	int32_t id;
	Task* parent;
	uint32_t state;
	uint32_t run_count;

	PageDirInfo pgdir;

	uint32_t time_start, time_end, time_user, time_kernel;
	int32_t counter;
	uint32_t priority;

	ChildList_t childs;

	//
	uint32_t free: 1;
	uint32_t kernel_mode: 1;
	uint32_t waiting_child: 1;
	uint32_t trap_in_signal: 1;
	uint32_t registers_saved: 1;
	uint32_t __ : 27;
	//

	SignalInfo signal;
	uint32_t alarm;
	AlarmList_t::node_t alarm_list_node;
	uint32_t sleep;

	int exit_code, exit_signal;

	void (*run_before_switch_f)(void*);
	void *run_before_switch_f_p;

	/* wait listesi */
	Task* wait_notify_next;

	SharedMemList_t shared_mem_list;

	inline void init() {
		list_node.init();
		childlist_node.init();
		childs.init();
		time_start = time_user = time_kernel = time_end = 0;
		run_count = counter = 0;
		alarm = 0;
		free = kernel_mode = waiting_child = 0;
		signal.init();
		wait_notify_next = NULL;

		/* shared_mem_list.init(); //shm_fork */
		id_hash_node.init();
		run_before_switch_f = NULL;
	}

	inline Trapframe* saved_registers() {
		if (signal.pending)
			return &registers_signal;

		return &registers_user;
	}

	inline Trapframe* registers() {
		if (registers_saved)
			return (trap_in_signal) ? &registers_signal : &registers_user;

		ASSERT(task_curr == this);
		return current_registers();
	}

	inline void save_new_registers() {
		if (trap_in_signal)
			registers_signal = *current_registers();
		else
			registers_user = *current_registers();
		registers_saved = 1;
	}

};

extern TaskIdHashTable_t task_id_ht;
extern TaskList_t task_zombie_list;
extern Task task0;

inline void task_trapret(Trapframe *tf) {
/*
 * task register struct'i stack olarak kullanilip pop ile registerlar
 * yukleniyor. (popal, es, ds, iret)
 */
	asm volatile("movl %0,%%esp\n\t"
				 "jmp trapret\n\t" /* traphandler.S */
				 :: "g" (tf) : "memory");
	PANIC("task_run: bu satir calismamali");
}

inline Task* SharedMemDesc::task() {
	return (Task*)((ptr_t)list_node.__list - offsetof(Task,shared_mem_list));
}


#endif /* TASK_H_ */
