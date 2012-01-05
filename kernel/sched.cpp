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

// signal.cpp
extern int send_signal(uint32_t sig, Task* t);
extern void check_signals();
//

void check_alarm();
void check_sleep_list();

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

// FIXME: schedule, baska bir fonksiyon icerisinden cagirilinca counter
// birikiyor. Buna bir cozum bulunmali.
void schedule() {
	cli();

	if (task_curr && !task_curr->registers_saved)
		task_curr->save_new_registers();

	if (task_curr && task_curr->run_before_switch_f) {
		task_curr->run_before_switch_f(task_curr->run_before_switch_f_p);
		task_curr->run_before_switch_f = NULL;
	}

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
			do {
				task_curr = first_priority_level->front();
				first_priority_level->pop_front();
				first_priority_level->push_back(&task_curr->list_node);

				task_curr->counter += task_curr->priority;
/*
 * Counter degeri priority/2 den kucukse taski calistirma. Bu sayede counter
 * eksi degerden, arti degere cikana kadar task bekler. counter'in eksi degere
 * dusmesinin sebebi calismasi izin verilenden fazla sure calismasidir.
 * Bu durumun gerceklesme ihtimali dusuk ama bu durumdan korumak icin bir
 * kontrol.
 * Bu durum Kernel modda cok fazla zaman gecirildiginde olusabilir.
 * Eger kernel modda intterruptlar acik ve kernel moddayken counter bittiginde
 * kernel registerlariyla task switch yapilirsa bu durum hic meydana gelmez.
 */
			} while (task_curr->counter < task_curr->priority/2);

/*
 * Sleep, pause, wait gibi sistem cagrilarinda task counteri bitmeden task
 * switch yapilmakta. Bu durumda sonra task counter birikip buyuk bir degere
 * ulasabilir. Bu problemi cozmek icin counter limiti kullanildi.
 *
 * Burada task counter limiti, oncelik seviyesine esit olarak kullanildi.
 */
			if (task_curr->counter > task_curr->priority)
				task_curr->counter = task_curr->priority;

			ASSERT(task_curr->state == Task::State_running);
			cr3_load(task_curr->pgdir.pgdir_pa);

			/* kernel modda switch yapilmissa devam et */
			if (task_curr->kernel_mode) {
				// printf(">> kernel mode switch\n");
				task_curr->kernel_mode = 0;
				task_curr->registers_kernel.regs.eax = 0;
				task_trapret(&task_curr->registers_kernel);
			}

			/* yeni gelen signalleri kontrol et */
			check_signals();

		} else {
			/* hic runnable task yoksa */
			task_curr = NULL;
			/* kesme gelene kadar bekle */
			sti();
			asm("hlt");
			cli();
		}
	} while (first_priority_level == NULL);

	/* secilmis task'i calistir */
	task_curr->run_count++;
	task_trapret(task_curr->saved_registers());
}

asmlink void sys_pause() {
	Trapframe *tf = task_curr->registers();

	remove_from_runnable_list(task_curr);

	task_curr->state = Task::State_interruptible;
	/*
	 * pause fonksiyonu sonlandiginda -1 degeri dondur:
	 * http://www.kernel.org/doc/man-pages/online/pages/man2/pause.2.html
	 */
	set_return(tf, -1);

	kernel_mode_task_switch();
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

	task_curr->counter--;
	// printf(">> counter: %d\n", task_curr->counter);

	if ((tf->cs & 3) == 3) {
		/* user modda timer kesmesi */
		task_curr->time_user++;
		// printf(">> do_timer (user mode)\n");
/* task'in suresi dolduysa (counter'i bittiyse) task switch yap */
		if (task_curr->counter < 0)
			schedule();
	} else {
		/* kernel modda timer kesmesi */
		ASSERT(tf != current_registers());
		task_curr->time_kernel++;
		// printf(">> do_timer (kernel mode)\n");
	}
}

asmlink void sys_alarm() {
	Trapframe* tf = task_curr->registers();
	unsigned int seconds = (unsigned int)get_param1(tf);

	uint32_t eflags = eflags_read();
	cli();

	// FIXME: hata kontrolu
	/* alarm listesindeyse listeden cikar */
	if (task_curr->alarm_list_node.__list)
		task_alarm_list.erase(&task_curr->alarm_list_node);

	if (seconds == 0) {
		eflags_load(eflags);
		return set_return(tf, 0);
	}

	task_curr->alarm = jiffies_to_seconds() + seconds;
	ASSERT(task_curr->alarm > jiffies_to_seconds());

	// FIXME: hata kontrolu
	/* alarm listesine sure siralamasina gore insert yap */
	AlarmList_t::iterator i;
	for (i = task_alarm_list.begin() ; i != task_alarm_list.end() ; i++) {
		if (i->value()->alarm > seconds)
			task_alarm_list.insert(i, &task_curr->alarm_list_node);
	}
	task_alarm_list.push_back(&task_curr->alarm_list_node);

	eflags_load(eflags);

	return set_return(tf, 0);
}

/** alarm suresi dolan tasklari kontrol eder */
void check_alarm() {
	uint32_t eflags = eflags_read();
	cli();

	const long seconds = jiffies_to_seconds();
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

	kernel_mode_task_switch();
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

	kernel_mode_task_switch();
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

	const long seconds = jiffies_to_seconds();

	TaskList_t::iterator i;
	for (i = task_sleep_list.begin() ; i != task_sleep_list.end() ; ) {
		Task *t = i->value();

		if (t->sleep > seconds)
			break;
		i++;

		task_sleep_list.erase(&t->list_node);
		ASSERT(t->list_node.is_free());
		t->sleep = 0;
		set_return(t->saved_registers(), 0);
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

	eflags_load(eflags);

	task_curr->state = Task::State_interruptible;
	task_curr->sleep = jiffies_to_seconds() + seconds;

	kernel_mode_task_switch();

	int r = task_curr->sleep - jiffies_to_seconds();

	if (task_curr->trap_in_signal)
		set_return(&task_curr->registers_signal, (r < 1) ? 0 : r);
	else
		set_return(&task_curr->registers_user, (r < 1) ? 0 : r);

	set_return(tf, (r < 1) ? 0 : r);
}


void kernel_mode_task_switch() {
	uint32_t eflags = eflags_read();
	cli();

#if 1
	push_registers(&task_curr->registers_kernel.regs);
# else
	/* FIXME: asagidaki kod gercek makinada calismiyor, neden acaba? */
	asm("pushal");
	task_curr->registers_kernel.regs = *(PushRegs*)esp_read();
	asm("popal");
#endif
	task_curr->registers_kernel.es = es_read();
	task_curr->registers_kernel.ds = ds_read();

	uint32_t eip = read_eip();
	if (eip != 0) {
		task_curr->registers_kernel.eip = eip;
		task_curr->registers_kernel.cs = cs_read();
		task_curr->registers_kernel.eflags = eflags_read();
		task_curr->registers_kernel.esp = esp_read();
		task_curr->registers_kernel.ss = ss_read();

		task_curr->kernel_mode = 1;
		schedule();
	} else {
		/* kernel modda iret yapildiginda esp yuklenmiyor, burdan yukluyoruz */
		esp_load(task_curr->registers_kernel.esp);

		/* task signal sebebiyle uyandirilmis olabilir, signalleri kontrol et */
		check_signals();
		if (task_curr->signal.pending) {
			/*
			 * once signal calistirilacak, bu fonksiyon calistirilmadan onceki
			 * user registerlari stackde duruyor, onlari kaydet
			 */
			if (!task_curr->registers_saved)
				task_curr->save_new_registers();
		}
	}
/*
 * buradan trapret yapilamaz, cunku yarim kalan sistem cagrisi tamamlanmali
 */
	eflags_load(eflags);
}
