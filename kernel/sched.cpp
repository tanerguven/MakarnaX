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

#include "task.h"
#include "sched.h"
#include "time.h"

#include <signal.h>
#include "signal.h"

void check_alarm();
void check_sleep_list();
void switch_to_task(Task *newtask) __attribute__ ((noinline));

/** runnable (ready) queue) */
TaskList_t __task_runnable_queue[41];

AlarmList_t task_alarm_list;
TaskList_t task_sleep_list;

void schedule_init() {
	task_alarm_list.init();
	task_sleep_list.init();

	for (int i = 1 ; i < 41 ; i++) {
		__task_runnable_queue[i].init();
	}

	printf(">> schedule init OK\n");
}

void schedule() {
	cli();
	ASSERT(task_curr);
	// printf(">> [%d] schedule\n", task_curr->id);

	if (task_curr && task_curr->run_before_switch_f) {
		task_curr->run_before_switch_f(task_curr->run_before_switch_f_p);
		task_curr->run_before_switch_f = NULL;
	}

	Task *task_next = NULL;
	TaskList_t *first_priority_level;
	do {
		check_alarm();
		check_sleep_list();

		/* process olan en yuksek oncelikli listeyi bul */
		first_priority_level = NULL;
		for (int i = 40 ; i > 0 ; i--) {
			if (__task_runnable_queue[i].size() > 0) {
				first_priority_level = &__task_runnable_queue[i];
				break;
			}
		}

		if (first_priority_level) {
			/* runnable listesinden task bul ve zaman ekle */
			task_next = first_priority_level->front();
			first_priority_level->pop_front();
			first_priority_level->push_back(&task_next->list_node);

			task_next->counter = task_next->priority;

			ASSERT(task_next->state == Task::State_running);
		} else {
			Task *t = task_curr;
			/* hic runnable task yoksa */

			/* FIXME: task_curr = NULL, optimizasyonu engellemek icin memset */
			memset(&task_curr, 0, sizeof(Task*));

			/* kesme gelene kadar bekle */
			sti();
			asm("hlt");
			cli();
			task_curr = t;
		}
	} while (first_priority_level == NULL);

	switch_to_task(task_next);

	ASSERT(!(eflags_read() & FL_IF));
	check_signals();

	/* signal yokken push yapilmis stack olmamali */
	ASSERT(!(!task_curr->signal.pending && task_curr->sigstack.size() != 0));

	// FIXME: signal oldugunda, sleep return 2 kere oluyor ???

	// printf(">> [%d] schedule OK\n", task_curr->id);
}


void run_first_task() {
	task_curr = task_id_ht.get(1);
	cr3_load(task_curr->pgdir.pgdir_pa);
	task_trapret(&task_curr->registers_user);
}

asmlink void sys_pause() {

	/* birden fazla signal varsa, pause sonrakinin calismasini engellememeli */
	if (task_curr->sigstack.size() > 1)
		return;

	Trapframe *tf = task_curr->registers();

	remove_from_runnable_list(task_curr);

	task_curr->state = Task::State_interruptible;
	/*
	 * pause fonksiyonu sonlandiginda -1 degeri dondur:
	 * http://www.kernel.org/doc/man-pages/online/pages/man2/pause.2.html
	 */
	set_return(tf, -1);

	schedule();
}

asmlink void sys_yield() {
	schedule();
}

/** timer kesmesi ile calistirilan fonksiyon */
asmlink void do_timer(Trapframe *tf) {
	ASSERT(!(eflags_read() & FL_IF));
	jiffies++;

	if (task_curr == NULL) {
		/* sistem bosta bekliyor */
		// TODO: bosta gecen sureyi hesaplamak icin sayac eklenebilir
		return;
	}

	ASSERT(task_curr && task_curr->state == Task::State_running);

	task_curr->counter--;

	if ((tf->cs & 3) == 3) {
		task_curr->time_user++;
	} else {
		ASSERT(tf != current_registers());
		task_curr->time_kernel++;
	}

	/* task'in suresi dolduysa (counter'i bittiyse) task switch yap */
	if (task_curr->counter < 0)
		schedule();
}

asmlink void sys_alarm() {
	Trapframe* tf = task_curr->registers();
	unsigned int seconds = (unsigned int)get_param1(tf);

	uint32_t eflags = eflags_read();
	cli();

	/* alarm listesindeyse listeden cikar */
	if (task_curr->alarm_list_node.__list) {
		AlarmList_t::iterator r;
		r = task_alarm_list.erase(&task_curr->alarm_list_node);
		ASSERT(r != AlarmList_t::error());
	}

	if (seconds == 0) {
		eflags_load(eflags);
		return set_return(tf, 0);
	}

	task_curr->alarm = jiffies_to_seconds() + seconds;
	ASSERT(task_curr->alarm > jiffies_to_seconds());

	/* alarm listesine sure siralamasina gore insert yap */
	AlarmList_t::iterator i;
	for (i = task_alarm_list.begin() ; i != task_alarm_list.end() ; i++) {
		if (i->value()->alarm > seconds) {
			AlarmList_t::iterator r;
			r = task_alarm_list.insert(i, &task_curr->alarm_list_node);
			ASSERT(r != AlarmList_t::error());
		}
	}
	task_alarm_list.push_back(&task_curr->alarm_list_node);

	eflags_load(eflags);

	return set_return(tf, 0);
}

/** alarm suresi dolan tasklari kontrol eder */
void check_alarm() {
	uint32_t eflags = eflags_read();
	cli();

	const uint32_t seconds = jiffies_to_seconds();
	AlarmList_t::iterator i;
	for (i = task_alarm_list.begin() ; i != task_alarm_list.end() ; ) {

		if (i->value()->alarm > seconds)
			break;

		Task *t = i->value();
		// printf(">> send alarm %d\n", t->id);
		send_signal(SIGALRM, t);
		i++;
		task_alarm_list.erase(i-1);
	}
	eflags_load(eflags);
}

/** bir kaynak uzerinde sleep yapar */
void sleep_interruptible(TaskList_t *list) {
	ASSERT(task_curr->state == Task::State_running);
	cli();

	remove_from_runnable_list(task_curr);
	list->push_back(&task_curr->list_node);

	task_curr->state = Task::State_interruptible;

	schedule();
}

/** bir kaynak uzerinde sleep yapan tasklardan ilkini uyandirir */
void wakeup_interruptible(TaskList_t *list) {
	ASSERT(!(eflags_read() & FL_IF));

	if (list->size() == 0)
		return;

	Task* t = list->front();

	ASSERT( list->pop_front() );

	t->state = Task::State_running;
	add_to_runnable_list(t);
}

void sleep_uninterruptible(TaskList_t *list) {
	ASSERT(!(eflags_read() & FL_IF));
	ASSERT(task_curr->state == Task::State_running);
	ASSERT(!(eflags_read() & FL_IF));

	remove_from_runnable_list(task_curr);
	task_curr->state = Task::State_uninterruptible;

	list->push_back(&task_curr->list_node);

	schedule();
}

void wakeup_uninterruptible(TaskList_t *list) {
	ASSERT(!(eflags_read() & FL_IF));

	Task* t = list->front();
	ASSERT( list->pop_front() );

	t->state = Task::State_running;
	add_to_runnable_list(t);
}

/** zamana gore sleep listesinde zamani dolan eleman var mi kontrol eder */
void check_sleep_list() {
	uint32_t eflags = eflags_read();
	cli();

	const uint32_t seconds = jiffies_to_seconds();

	TaskList_t::iterator i;
	for (i = task_sleep_list.begin() ; i != task_sleep_list.end() ; ) {
		Task *t = i->value();

		if (t->sleep > seconds)
			break;
		i++;

		task_sleep_list.erase(&t->list_node);
		ASSERT(t->list_node.is_free());
		t->sleep = 0;
		t->state = Task::State_running;
		add_to_runnable_list(t);
	}

	eflags_load(eflags);
}

/** zamana gore sleep yapar */
asmlink void sys_sleep() {
	Trapframe* tf = task_curr->registers();
	unsigned int seconds = (unsigned int)get_param1(tf);

	if (seconds == 0)
		return set_return(tf, 0);

	uint32_t eflags = eflags_read();
	cli();

	/* runnable kuyrugundan sil */
	remove_from_runnable_list(task_curr);
	/* sleep listesine zaman sirasina gore insert yap */
	TaskList_t::iterator i;
	for (i = task_sleep_list.begin() ; i != task_sleep_list.end() ; i++) {
		if (i->value()->sleep > seconds)
			task_sleep_list.insert(i, &task_curr->list_node);
	}
	task_sleep_list.push_back(&task_curr->list_node);

	task_curr->state = Task::State_interruptible;
	task_curr->sleep = jiffies_to_seconds() + seconds;

	eflags_load(eflags);

	schedule();
	// printf(">> [%d] sleep return\n", task_curr->id);

	int r = task_curr->sleep - jiffies_to_seconds();

	return set_return(task_curr->registers(), (r < 1) ? 0 : r);
}

void switch_to_task(Task *newtask) {
	ASSERT(!(eflags_read() & FL_IF));
	ASSERT(task_curr);

	task_curr->k_esp = esp_read();
	cr3_load(newtask->pgdir.pgdir_pa);
	esp_load(newtask->k_esp);
	task_curr = newtask;

	task_curr->run_count++;
	if (newtask->ran==0) {
		/* task ilk kez calisiyorsa, user modda baslat */
		newtask->ran = 1;
		task_trapret(&newtask->registers_user);
	}
}
