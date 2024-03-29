/*
 * Copyright (C) 2011,2012 Taner Guven <tanerguven@gmail.com>
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

#include <kernel/kernel.h>
#include <types.h>
#include <wmc/list.h>
#include <wmc/idhashtable.h>

#include "trap.h"
#include "memory/virtual.h"
#include "signal.h"
#include "ipc/ipc.h"
#include "fs/task_fs.h"

#define DEFAULT_PRIORITY 3

define_list(struct Task, AlarmList_t);
define_list(struct Task, ChildList_t);
define_list(struct Task, TaskList_t);
define_id_hash(struct Task, TaskIdHashTable_t);

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

	/* registers: kernel stack uzerinden kullaniliyor */

	int32_t id;
	Task* parent;
	uint32_t state;
	uint32_t run_count;

	PageDirInfo pgdir;

	/* task switch icin degiskenler */
	uint32_t k_esp, k_ebp, k_eip;

	uint32_t time_start, time_end, time_user, time_kernel;
	int32_t counter;
	int32_t priority;

	ChildList_t childs;

	//
	uint32_t free: 1;
	uint32_t waiting_child: 1;
	uint32_t popped_kstack: 1;
	uint32_t __ : 29;
	//

	SignalStack_t sigstack;
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

	Task_FS fs;
	struct spinlock lock;

	/* pushcli/popcli icin degiskenler */
	int n_cli;
	uint32_t before_pushcli;

	inline void init();
	inline Trapframe* registers();
};


extern TaskIdHashTable_t task_id_ht;
extern TaskList_t task_zombie_list;
extern struct Task* task_curr;
extern struct Task task0;

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


inline Trapframe* current_registers() {
	ASSERT_DTEST(task_curr);
	if (task_curr->popped_kstack)
		return (Trapframe*)va2kaddr(MMAP_KERNEL_STACK_2_TOP - sizeof(Trapframe));
	else
		return (Trapframe*)va2kaddr(MMAP_KERNEL_STACK_TOP - sizeof(Trapframe));
}

inline void Task::init() {
	list_node.init();
	childlist_node.init();
	childs.init();
	time_start = time_user = time_kernel = time_end = 0;
	run_count = counter = 0;
	alarm = 0;
	free = waiting_child = 0;
	signal.init();
	wait_notify_next = NULL;

	/* shared_mem_list.init(); //shm_fork */
	id_hash_node.init();
	run_before_switch_f = NULL;
	sigstack.init();
	spinlock_init(&this->lock);
	n_cli = 0;
}

inline Trapframe* Task::registers() {
	if (task_curr == this)
		return current_registers();
	PANIC("task->registers() hatali");
	return NULL;
}

#endif /* TASK_H_ */
