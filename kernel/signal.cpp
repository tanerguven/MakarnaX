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

int send_signal(uint32_t sig, Task* t) {
	if (!t || sig > 32)
		return -EINVAL;

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
		printf("%d\n", t->state);
		PANIC("--");
	}

	return 0;
}

void check_signals() {
	ASSERT(!(eflags_read() & FL_IF));
	ASSERT(task_curr->pgdir.pgdir_pa == cr3_read());

	if (task_curr->signal.sig) {
		uint32_t* esp;

		/* signal registeri ilk kez kullaniliyorsa, user registerini kopyala */
		if (task_curr->signal.pending == 0)
			task_curr->registers_signal = task_curr->registers_user;
		esp = (uint32_t*)uaddr2kaddr(task_curr->registers_signal.esp);

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

				task_curr->signal.sig &= ~_S(sig);
				task_curr->signal.pending |= _S(sig);

				/*
				 * signal handler fonksiyon olarak calisacak.
				 * return eip degerine belirlenmis page fault adresini ata.
				 * Bu adreste paga fault olunca signal_return fonksiyonu calisir
				 */
				esp[-1] = task_curr->registers_signal.eip; //bir onceki eip
				esp[-2] = sig;
				esp[-3] = va2uaddr(0xfffffffe); // fault eip
				esp-=3;
				task_curr->registers_signal.esp = kaddr2uaddr((uint32_t)esp);
				task_curr->registers_signal.eip = task_curr->signal.action[sig].handler;
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

	/* signal fonksiyonlarindan siradakini calistir */
	if (tf->esp != task_curr->registers_user.esp) {
		task_trapret(tf);
		PANIC("--");
	}

	/* signal kalmadiysa normal calistir */
	task_trapret(&task_curr->registers_user);
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
