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
 * schedule fonksiyonlarinda senkronizasyon yonetmi:
 *
 * buradaki fonksiyonlar, process listeleri uzerinde degisiklik yapiyor genel
 * olarak islemler kisa ve birden fazla cpu durumunda butun cpulari ilgilendiriyor
 *
 * paylasilan veriler kullanilirken sistem genelinde cli yapiliyor
 */

#include <kernel/kernel.h>
#include <kernel/syscall.h>

#include "task.h"
#include "sched.h"
#include "time.h"

#include <signal.h>
#include "signal.h"

// task.cpp
extern void switch_to_task(Task *newtask);

void check_alarm();
void check_sleep_list();

/** runnable (ready) queue) */
TaskList_t __task_runnable_queue[41];

AlarmList_t task_alarm_list;
TaskList_t task_sleep_list;

void add_to_runnable_list(Task* t) {
/*
 * task listelerinde degisiklik yapilacaksa sistem genelinde cli yapilmali
 * cunku listede degisiklik yapmak icin hem task hem de listede lock yapilmali
 * cli daha mantikli
 */
	pushcli();
	ASSERT_DTEST(t->list_node.is_free());

	t->state = Task::State_running;
	ASSERT( __task_runnable_queue[t->priority].push_back(&t->list_node) );

	popif();
}

void remove_from_runnable_list(Task* t) {
	pushcli();
	ASSERT_DTEST(t->list_node.__list == &__task_runnable_queue[t->priority]);
	ASSERT_DTEST(t->state == Task::State_running);

	ASSERT( __task_runnable_queue[t->priority].erase(&t->list_node)
			!= __task_runnable_queue[t->priority].error());

	ASSERT_DTEST(t->list_node.is_free());
	popif();
}

void schedule_init() {
	task_alarm_list.init();
	task_sleep_list.init();

	for (int i = 1 ; i < 41 ; i++) {
		__task_runnable_queue[i].init();
	}

	print_info(">> schedule init OK\n");
}

Task* find_runnable_task() {
	Task *task_next = NULL;
	TaskList_t *first_priority_level;

	pushcli();

	/* process olan en yuksek oncelikli listeyi bul */
	first_priority_level = NULL;
	for (int i = 40 ; i > 0 ; i--) {
		if (__task_runnable_queue[i].size() > 0) {
			first_priority_level = &__task_runnable_queue[i];
			break;
		}
	}

	if (first_priority_level) {
		bool b;
		/* runnable listesinden task bul ve zaman ekle */
		task_next = first_priority_level->front();
		b = first_priority_level->pop_front();
		ASSERT( b );
		b = first_priority_level->push_back(&task_next->list_node);
		ASSERT( b );

		task_next->counter = task_next->priority;
		ASSERT(task_next->state == Task::State_running);
	}

	popif();
	ASSERT_int_enable();

	return task_next;
}

void schedule() {

	ASSERT_int_enable();

	if (task_curr->run_before_switch_f) {
		task_curr->run_before_switch_f(task_curr->run_before_switch_f_p);
		task_curr->run_before_switch_f = NULL;
	}

	Task *task_next = NULL;

/*
 * TODO: check alarm ve sleep'i bunlari do_timer icerisinde tasimak daha
 * mantikli gibi. Bu fonksiyonlar timer ile ilgili, timer saat degistiginde
 * calisiyor. Schedule her zaman timer ile calismiyor.
 */
	check_alarm();
	check_sleep_list();
	ASSERT_int_enable();
	task_next = find_runnable_task();

	if (!task_next) {
		PANIC("bu durum yok, kernel task beklemeye gecemez.\n"
			  "runnable kuyrugu bos olmamali");
	}

	cli();
	switch_to_task(task_next);
	sti();

	check_signals();

	/* signal yokken push yapilmis stack olmamali */
	ASSERT(!(!task_curr->signal.pending && task_curr->sigstack.size() != 0));

	// FIXME: signal oldugunda, sleep return 2 kere oluyor ???

	// printf(">> [%d] schedule OK\n", task_curr->id);
}

SYSCALL_DEFINE0(pause) {
	/* birden fazla signal varsa, pause sonrakinin calismasini engellememeli */
	if (task_curr->sigstack.size() > 1)
		return;

	pushcli();
	remove_from_runnable_list(task_curr);
	task_curr->state = Task::State_interruptible;
	popif();

	/*
	 * pause fonksiyonu sonlandiginda -1 degeri dondur:
	 * http://www.kernel.org/doc/man-pages/online/pages/man2/pause.2.html
	 */
	SYSCALL_RETURN(-1);

	schedule();
}
SYSCALL_END(pause)


SYSCALL_DEFINE0(yield) {
	schedule();
	SYSCALL_RETURN(0);
}
SYSCALL_END(yield)


/** timer kesmesi ile calistirilan fonksiyon */
asmlink void do_timer(Trapframe *tf) {
	ASSERT_int_enable();

	jiffies++;

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

SYSCALL_DEFINE1(alarm, unsigned int, seconds) {
	pushcli();

	/* alarm listesindeyse listeden cikar */
	if (task_curr->alarm_list_node.__list) {
		AlarmList_t::iterator r;
		r = task_alarm_list.erase(&task_curr->alarm_list_node);
		ASSERT(r != AlarmList_t::error());
	}

	if (seconds == 0) {
		popif();
		return SYSCALL_RETURN(0);
	}

	task_curr->alarm = jiffies_to_seconds() + seconds;

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

	popif();

	return SYSCALL_RETURN(0);
}
SYSCALL_END(alarm)


/** alarm suresi dolan tasklari kontrol eder */
void check_alarm() {
	pushcli();

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
	popif();
}

/** bir kaynak uzerinde sleep yapar */
void sleep_interruptible(TaskList_t *list) {
	pushcli();
	ASSERT_DTEST(task_curr->state == Task::State_running);

	remove_from_runnable_list(task_curr);
	list->push_back(&task_curr->list_node);
	task_curr->state = Task::State_interruptible;

	popif();
	schedule();
}

/** bir kaynak uzerinde sleep yapan tasklardan ilkini uyandirir */
void wakeup_interruptible(TaskList_t *list) {
	pushcli();
	Task* t = list->front();
	if (t == list->end()->value()) {
		popif();
		return;
	}
	ASSERT( list->pop_front() );
	add_to_runnable_list(t);
	popif();
}

void sleep_uninterruptible(TaskList_t *list) {
	pushcli();
	ASSERT_DTEST(task_curr->state == Task::State_running);

	remove_from_runnable_list(task_curr);
	task_curr->state = Task::State_uninterruptible;
	list->push_back(&task_curr->list_node);
	popif();

	schedule();
}

void wakeup_uninterruptible(TaskList_t *list) {
	pushcli();

	Task* t = list->front();
	ASSERT( list->pop_front() );

	add_to_runnable_list(t);
	popif();
}

/** zamana gore sleep listesinde zamani dolan eleman var mi kontrol eder */
void check_sleep_list() {
	pushcli();

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
		add_to_runnable_list(t);
	}

	popif();
}


SYSCALL_DEFINE1(sleep, unsigned int, seconds) {
	if (seconds == 0)
		return SYSCALL_RETURN(0);

	pushcli();

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

	popif();

	schedule();
	// printf(">> [%d] sleep return %d\n", task_curr->id, task_curr->sleep);

	int r = task_curr->sleep - jiffies_to_seconds();

	return SYSCALL_RETURN((r < 1) ? 0 : r);
}
SYSCALL_END(sleep)
