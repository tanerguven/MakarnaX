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

#include <stdio.h>
#include "panic.h"

#include <signal.h>
#include <sys/wait.h>

#include "task.h"
#include "sched.h"
#include "time.h"
#include "memory/virtual.h"

#include <wmc/idhashtable.h>

// task.cpp
extern void task_free(struct Task *t);
//

// signal.cpp
int send_signal(uint32_t sig, Task* t);
//



/*
 * TODO: parent prosesin, child prosesten once sonlanmasi durumu
 * TODO: birden fazla child prosesin ayni anda sonlanmasi durumunda bilgi
 * parent processe nasil aktarilacak. (sys_wait'de 1 id return yapilabiliyor)
 * TODO: forktest test2 uzerinde denemeler yapilmali
 */

void notify_parent(Task *t) {
	ASSERT(t->parent);
	ASSERT(t->parent != t);
	int r;

	r = send_signal(t->exit_signal, t->parent);
	ASSERT(r > -1);

	// FIXME: bu durum incelenmeli
	/* child bekliyorsa notify listesine ekle */
	if (t->parent->waiting_child) {
		// FIXME: bir liste yapisi yapilmali
		t->wait_notify_next = t->parent->wait_notify_next;
		t->parent->wait_notify_next = t;
	}
}

asmlink void do_exit(int code) {
	cli();

	if (task_curr->id == 1)
		PANIC(">> exit task 1");

	remove_from_runnable_list(task_curr);
	task_curr->state = Task::State_zombie;
	task_zombie_list.push_back(&task_curr->list_node);

	task_free(task_curr);
	task_curr->time_end = jiffies;
	task_curr->exit_code = code;

	// FIXME: ???
	if (task_curr->exit_signal == 0)
		task_curr->exit_signal = SIGCHLD;

	notify_parent(task_curr);
	printf(">> [%d] exit OK\n", task_curr->id);

	schedule();
}

asmlink void sys_exit() {
	Trapframe* tf = task_curr->registers();
	int error_code = get_param1(tf);

	do_exit((error_code&0xff) << 8);
	return set_return(tf, 0);
}

asmlink void sys_kill() {
	Trapframe* tf = task_curr->registers();
	int pid = get_param1(tf);
	int sig = get_param2(tf);

	uint32_t eflags = eflags_read();
	cli();

	Task *t = task_id_ht.get(pid);

	eflags_load(eflags);

	if (t == NULL)
		return set_return(tf, -1);
	send_signal(sig, t);
	return set_return(tf, 0);
}

/*
 * child process'in bitmesini bekle ve child process idsini dondur.
 * child process yoksa -1 dondur.
 */
asmlink void sys_wait() {
	Trapframe* tf = task_curr->registers();
	int *state = (int*)get_param1(tf);

	ASSERT(task_curr);

	if ( task_curr->pgdir.verify_user_addr(state, 4, PTE_U) < 0 ) {
		printf(">> sys_wait not verified: 0x%08x - 0x%08x\n", state, state+1);
		do_exit(111);
	}
	state = (int*)uaddr2kaddr((uint32_t)state);

	uint32_t eflags = eflags_read();
	cli();

	if (task_curr->childs.size() == 0) {
		eflags_load(eflags);
		return set_return(tf, -1);
	}

	/* cagriyi yapan taski runnable_list'den cikar */
	remove_from_runnable_list(task_curr);
	task_curr->state = Task::State_interruptible;
	task_curr->waiting_child = 1;

	eflags_load(eflags);

	kernel_mode_task_switch();

	task_curr->waiting_child = 0;
	Task *t = task_curr->wait_notify_next;
	ASSERT(t);
	task_curr->wait_notify_next = t->wait_notify_next;

	/* return ve state degiskenlerini ata */
	*state = t->exit_code;

	return set_return(task_curr->registers(), t->id);
}
