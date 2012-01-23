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
#include "../panic.h"

#include "../trap.h"
#include "../task.h"
#include "../sched.h"

#include "../kernel.h"

/*
 * TODO: SemInfo icin alinan bellek geri verilmeli
 */

/*
 * kaynaklar:
 * http://linux.die.net/man/3/sem_init
 */

struct SemInfo {
	TaskList_t wait_list;
	int value;
	struct sem_t *sem;
};

struct sem_t {
	SemInfo *info;
};

void sem_init() {
	ASSERT(sizeof(sem_t) < 32);
}

void sys_semget() {
	Trapframe *tf = task_curr->registers();
	sem_t *sem = (sem_t*)get_param2(tf);
	/* pshared parametresi desteklenmiyor */
	// int pshared = (int)get_param3(tf);
	unsigned int value = get_param4(tf);

	if ( task_curr->pgdir.verify_user_addr(sem, sizeof(sem_t*), PTE_U) < 0 ) {
		// FIXME: adres hatasi olmasi durumda process sonlanmali mi, hata mi dondurulmeli
		printf(">> sys_semget not verified: 0x%08x - 0x%08x\n", sem, sem+1);
		return set_return(tf, -1);
	}

	sem = (sem_t*)uaddr2kaddr((uint32_t)sem);

	uint32_t eflags = eflags_read();
	cli();

	// FIXME: kullanimdan sonra free yapilmali
	sem->info = (SemInfo*)kmalloc(sizeof(SemInfo));
	sem->info->wait_list.init();
	sem->info->value = value;
	sem->info->sem = sem;

	eflags_load(eflags);

	return set_return(tf, 0);
}

void sys_sem_destroy() {
	Trapframe *tf = task_curr->registers();
	// sem_t *sem = (sem_t*)get_param2(tf);

	PANIC("fonksiyon tamamlanmadi");

	return set_return(tf, 0);
}

void do_sempost(void*);

void sys_semwait() {
	Trapframe *tf = task_curr->registers();
	struct sem_t *sem = (struct sem_t*)get_param2(tf);
	sem = (sem_t*)uaddr2kaddr((uint32_t)sem);

	ASSERT(sem->info->sem == sem);
	SemInfo *sem_info = sem->info;

	uint32_t eflags = eflags_read();
	cli();

	if (sem_info->value > 0) {
#if __CONF_semaphore_single_cpu_optimization == 1
		if (task_curr->run_before_switch_f && task_curr->counter == 0) {
			sleep_uninterruptible(&sem_info->wait_list);
		} else {
			sem_info->value--;
		}
# else
		sem_info->value--;
#endif
		ASSERT(sem_info->value == 0);
	} else {
		ASSERT(task_curr->run_before_switch_f == NULL);
		set_return(task_curr->registers(), 0);
		sleep_uninterruptible(&sem_info->wait_list);
	}

	eflags_load(eflags);

	return set_return(task_curr->registers(), 0);
}


#if __CONF_semaphore_single_cpu_optimization == 1
// TODO: birden fazla semafor olabilir, liste kullanilmali
void do_sempost(void *p) {
	SemInfo *sem_info = (SemInfo*)p;
	sem_info->value--;
	// printf("value %d\n", sem->value);
	ASSERT(sem_info->value == 0);
	wakeup_uninterruptible(&sem_info->wait_list);
}
#endif

void sys_sempost() {
	Trapframe *tf = task_curr->registers();
	struct sem_t *sem = (struct sem_t*)get_param2(tf);
	sem = (sem_t*)uaddr2kaddr((uint32_t)sem);

	ASSERT(sem->info->sem == sem);
	SemInfo *sem_info = sem->info;

	uint32_t eflags = eflags_read();
	cli();

	if (sem_info->wait_list.size() > 0) {
	  ASSERT(sem_info->value == 0);
#if __CONF_semaphore_single_cpu_optimization == 1
/*
 * Task counteri bitmesine yakin wakeup yap. Boylece calisma suresinde semafor
 * sebebiyle kesintiye ugramaz. Her seferinde semaforu ele gecirir.
 * Suresinin bitmesine yakin semaforu kaybeder ve sleep durumuna gecer.
 *
 * Bu yontem tek islemci yada cekirdek oldugunda gecerli, birden cok islemci
 * varsa daha farkli bir yontem kullanilmali.
 */
		if (task_curr->counter > 0) {
// FIXME: bu yontem yerine taskin bosa sakladigi semaforlarin listesini tutsak
// ve task switch sirasinda serbest biraksak daha kolay olur
			sem_info->value++;
			task_curr->run_before_switch_f = do_sempost;
			task_curr->run_before_switch_f_p = sem_info;
		} else {
			if (task_curr->run_before_switch_f)
				task_curr->run_before_switch_f = NULL;
			wakeup_uninterruptible(&sem_info->wait_list);
		}
# else
/*
 * Her seferinde wakeup yapmak, semafor cok kullanildigi zaman cok fazla task
 * switche sebep oluyor. Saniyede milyonlarca task switch yapilip, task switch
 * islemiyle islemci bosa harcanabilir.
 */
		wakeup_uninterruptible(&sem_info->wait_list);
#endif
	} else {
		sem_info->value++;
	}

	eflags_load(eflags);

	return set_return(task_curr->registers(), 0);
}

void sys_semgetvalue() {
	Trapframe *tf = task_curr->registers();
	// struct sem_t *sem = (struct sem_t*)get_param2(tf);
	// int *val = (int*)get_param3(tf);

	printf(">> sys_semgetvalue\n");

	return set_return(tf, -1);
}

void sys_semop() {
	Trapframe *tf = task_curr->registers();
	struct sem_t *sem = (struct sem_t*)get_param2(tf);

	printf("sys_semop\n");
	printf(">> sem: %08x\n", sem);

	return set_return(tf, -1);
}

void sys_semctl() {
	printf("sys_semctl\n");
	PANIC("yok");
	return set_return(task_curr->registers(), -1);
}

// TODO: --
// uint32_t mem_sem() {

// }
