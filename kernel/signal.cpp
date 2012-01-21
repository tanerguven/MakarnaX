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
#include <types.h>
#include "task.h"
#include "sched.h"

#include <errno.h>
#include <signal.h>

/*
 * TODO: signal mask eklenmeli
 */

// task_exit.cpp
asmlink void do_exit(int);
extern void notify_parent(Task *t);
//

#define _S(x) (1<<x)

/* SIGKILL VE SIGSTOP haricindekiler */
#define SIG_BLOCKABLE (~((_S(SIGKILL)) | (_S(SIGSTOP))))

#define SIG_DI (_S(SIGCHLD) | _S(SIGURG))

#define SIG_DT (\
	_S(SIGALARM) | _S(SIGHUP) | _S(SIGINT) | _S(SIGKILL)\
	| _S(SIGPIPE) | _S(SIGTERM) | _S(SIGUSR1) | _S(SIGUSR2)\
	| _S(SIGPOLL) | _S(SIGPROF) | _S(SIGVTALARM)\
		)

#define SIG_DA (\
	_S(SIGABRT) | _S(SIGBUS) | _S(SIGFPE) | _S(SIGILL)\
	| _S(SIGQUIT) | _S(SIGSEGV) | _S(SIGSYS) | _S(SIGTRAP)\
	| _S(SIGXCPU) | _S(SIGXFSZ)\
		)

#define SIG_DS (_S(SIGSTOP) | _S(SIGTSTP) | _S(SIGTTIN) | _S(SIGTTOU))

#define SIG_DC (_S(SIGCONT))

typedef void (*sighandler_t)(int);

enum SignalHandler {
	SIG_DFL = 0,
	SIG_IGN = 1,
	SIG_ERR = -1,
};

void copy_stack(uint32_t addr);
static void push_stack();
static void pop_stack();

int send_signal(uint32_t sig, Task* t) {
	if (!t || sig > 32)
		return -EINVAL;

	if (t->signal.pending & _S(sig))
		return 0;

	t->signal.sig |= _S(sig);

	if (t->state == Task::State_interruptible) {
		if (task_curr->signal.action[sig].handler == SIG_DFL) {
/*
 * FIXME: signal handler default ve ignore olursa taski uyandirmali miyiz?
 * wait icin uyandirmak gerekiyor, child signal aldiginda SIGCHLD uyanmali
 * diger durumlar ?
 */
			if ((SIG_DI | SIG_DC) & _S(sig)) {
				t->state = Task::State_running;
				add_to_runnable_list(t);
				return 0;
			}
		}
		uint32_t eflags = eflags_read();
		cli();
		/* taski bekledigi listeden sil ve uyandir */
		if (t->list_node.__list)
			t->list_node.__list->erase(&t->list_node);
		t->state = Task::State_running;
		add_to_runnable_list(t);
		eflags_load(eflags);
	} else if (t->state == Task::State_running) {
		// FIXME: ?
	} else {
		/*
 		 * FIXME: --
		 * uninterruptible durumu henuz kullanilmadi
		 * diger durumlar da tanimlanmali
		 */
		printf("[%d] %d\n", t->id, t->state);
		PANIC("--");
	}

	return 0;
}

void check_signals() {
	ASSERT(!(eflags_read() & FL_IF));
	ASSERT(task_curr->pgdir.pgdir_pa == cr3_read());

	if (task_curr->signal.sig) {
		Trapframe *tf = NULL;

		// FIXME: 32'lik dongu yerine daha mantikli bir cozum bulunmali
		for (int sig = 31 ; (sig > 0) && (task_curr->signal.sig) ; sig--) {

			if (task_curr->signal.sig & _S(sig)) {

				if (task_curr->signal.action[sig].handler == SIG_DFL) {
					/* task 1, default olarak signal almiyor */
					if (task_curr->id == 1)
						continue;

					if (SIG_DI & _S(sig)) {
						/* ignore */
						continue;
					} else if (SIG_DS & _S(sig)) {
						task_curr->state = Task::State_stopped;
						remove_from_runnable_list(task_curr);
						task_curr->exit_code = (sig << 8) | 0x7f;
						notify_parent(task_curr);
						schedule();
					} else if (SIG_DC & _S(sig)) {
						// TODO: continued durumu icin birseyler yapilmali
						continue;
					}
					/*(SIG_DA & _S(sig)) | (SIG_DT & _S(sig))*/
					task_curr->signal.sig |= _S(sig);
					do_exit(sig);
					PANIC("--");
				}

				push_stack();
/*
 * pop signal stack sonrasi buradan devam ediliyor, asagidaki kontrol signal
 * tamamlandiktan sonra buradan cikip normal calismaya devam etmesi icin
 */
				if (!task_curr->signal.sig)
					return;

				tf = current_registers();
				uint32_t *esp = (uint32_t*)uaddr2kaddr(tf->esp);

				task_curr->signal.sig &= ~_S(sig);
				task_curr->signal.pending |= _S(sig);

				/*
				 * signal handler fonksiyon olarak calisacak.
				 * return eip degerine belirlenmis page fault adresini ata.
				 * Bu adreste paga fault olunca signal_return fonksiyonu calisir
				 */
				esp[-1] = tf->eip; //bir onceki eip
				esp[-2] = sig;
				esp[-3] = va2uaddr(0xfffffffe); // fault eip
				esp-=3;
				tf->esp = kaddr2uaddr((uint32_t)esp);
				tf->eip = task_curr->signal.action[sig].handler;
			}
		}
	}
}

/* user signal handler fonksiyonundan return yapildiginda calisir */
void signal_return(Trapframe *tf) {
	ASSERT(!(eflags_read() & FL_IF));

	uint32_t* esp = (uint32_t*)uaddr2kaddr(tf->esp);

/*
 * schedule.cpp, schedule()
 * esp[-1] = n->registers.eip;     //-> esp[1]
 * esp[-2] = sig;                  //-> esp[2]
 * esp[-3] = va2uaddr(0xfffffffe); //-> fonksiyondan ret ile eip'ye yuklendi
 * esp -= 3;
 */
	task_curr->signal.pending &= ~_S(esp[0]); //signal tamamlandi, biti sifirla
	tf->eip = esp[1];
	tf->esp += 8;

	/* pop ile push yapilan yere atlaniyor :) */
	pop_stack();
}

asmlink void sys_signal() {
	Trapframe* tf = current_registers();
	int signum = (int)get_param1(tf);
	uint32_t handler = get_param2(tf);

	if (signum < 0 || signum > 32 || signum == SIGKILL || signum == SIGSTOP)
		return set_return(tf, -EINVAL);

	//FIXME: handler icin adres kontrolu

	task_curr->signal.action[signum].handler = handler;

	return set_return(tf, 0);
}

// FIXME: uygun yere tasi
void copy_stack(uint32_t addr) {
	ASSERT(isRounded(addr));

	memcpy((void*)va2kaddr(addr),
		   (void*)va2kaddr(MMAP_KERNEL_STACK_BASE), 0x1000);

	for (uint32_t* ebp = (uint32_t*)ebp_read() ;
		 va2kaddr(MMAP_KERNEL_STACK_TOP) - (uint32_t)ebp < 0x1000 ;
		 ebp = (uint32_t*)*ebp)
	{
		*(uint32_t*)(addr + ((uint32_t)ebp % 0x1000)) = *ebp - 0x2000;
	};
}

static void push_stack() {
	// printf(">> push stack to %d\n", task_curr->kstack_c);
	ASSERT(task_curr->kstack_c < 32);
	ASSERT(!task_curr->popped_kstack);
	int r;
	r = page_alloc(&task_curr->kstack[task_curr->kstack_c]);
	ASSERT(r > -1);
	r = task_curr->pgdir.page_insert(task_curr->kstack[task_curr->kstack_c],
									 MMAP_KERNEL_STACK_BASE - 0x2000,
									 PTE_P | PTE_W);
	ASSERT(r > -1);
	copy_stack(MMAP_KERNEL_STACK_BASE - 0x2000);
	task_curr->kstack_esp[task_curr->kstack_c] = esp_read() - 0x2000;
	task_curr->ssleep[task_curr->kstack_c] = task_curr->sleep;
	task_curr->kstack[task_curr->kstack_c]->refcount_inc();
	task_curr->kstack_c++;
}

static void pop_stack() {
	ASSERT(!task_curr->popped_kstack);
	task_curr->kstack_c--;
	// printf(">> pop stack from %d\n", task_curr->kstack_c);
	ASSERT(task_curr->kstack_c > -1);
	task_curr->sleep = task_curr->ssleep[task_curr->kstack_c];
	task_curr->pgdir.page_insert(task_curr->kstack[task_curr->kstack_c],
								 MMAP_KERNEL_STACK_BASE - 0x2000,
								 PTE_P | PTE_W);
	task_curr->kstack[task_curr->kstack_c]->refcount_dec();
	task_curr->popped_kstack = 1;
	esp_load(task_curr->kstack_esp[task_curr->kstack_c]);
}
