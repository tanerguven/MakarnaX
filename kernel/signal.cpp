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
 * signal fonksiyonlarinda senkronizasyon yontemi:
 *
 * process'in kendisi disindaki processlerin eristigi fonksiyonlarda cli yapmak
 * yani sadece send_signal fonksiyonunda kullaniliyor
 *
 * diger durumlarda, process kendi verileri uzerinde degisiklik yaptigi icin
 * senkronizasyona gerek yok
 */

#include "task.h"
#include "sched.h"

#include <errno.h>
#include <signal.h>

#include <kernel/syscall.h>
#include <kernel/kernel.h>

/*
 * TODO: signal mask eklenmeli
 */

// task_exit.cpp
extern void notify_parent(Task *t);

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

set_list_offset(struct SignalState, SignalStack_t, list_node);

static void push_stack();
static void pop_stack();

int send_signal(uint32_t sig, Task* t) {
	if (!t || sig > 32)
		return -EINVAL;

	pushcli();

	if (t->signal.pending & _S(sig)) {
		popif();
		return 0;
	}

	t->signal.sig |= _S(sig);

	if (t->state == Task::State_interruptible) {
		if ((task_curr->signal.action[sig].handler == SIG_DFL)
			&& (SIG_DI | SIG_DC) & _S(sig)) {
/*
 * TODO: bu durumda task uyandirilmali mi? Bu durumlar arastirilmali.
 * Linux'da testler:
 * - sleep durumunda SIGCHLD ile uyanmiyor
 * - wait child durumunda SIGCHLD ile uyaniyor
 */
			/* FIXME: gecici cozum, wait child durumunda degilse ignore yap */
			if (!t->waiting_child) {
				popif();
				return 0;
			}
		}

		uint32_t eflags = eflags_read();
		cli();
		/* taski bekledigi listeden sil ve uyandir */
		if (t->list_node.__list)
			t->list_node.__list->erase(&t->list_node);
		add_to_runnable_list(t);
		eflags_load(eflags);
	} else if (t->state == Task::State_running) {
		// FIXME: ?

	} else if (t->state == Task::State_zombie) {
		/* bekleyen prosese kill yapilinca gerceklesiyor */

	} else {
		/*
 		 * FIXME: --
		 * uninterruptible durumu henuz kullanilmadi
		 * diger durumlar da tanimlanmali
		 */
		print_error("[%d] %d\n", t->id, t->state);
		PANIC("--");
	}

	popif();
	return 0;
}

void check_signals() {
	ASSERT_int_enable();

	// FIXME: bu testler kaldirilmali
	uint32_t cr3; read_reg(%cr3, cr3);
	ASSERT(task_curr->pgdir.pgdir_pa == cr3);

	pushcli();
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
						popif();
						schedule();
						PANIC("--");
					} else if (SIG_DC & _S(sig)) {
						// TODO: continued durumu icin birseyler yapilmali
						continue;
					}
					/*(SIG_DA & _S(sig)) | (SIG_DT & _S(sig))*/
					task_curr->signal.sig |= _S(sig);
					popif();
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
				// TODO: galiba tf->eip kullanilmiyor, bunu eski yontemde kullanmistim
				esp[-1] = tf->eip; //bir onceki eip
				esp[-2] = sig;
				esp[-3] = va2uaddr(0xfffffffe); // fault eip
				esp-=3;
				tf->esp = kaddr2uaddr((uint32_t)esp);
				tf->eip = task_curr->signal.action[sig].handler;
			}
		}
	}
	popif();
}

/* user signal handler fonksiyonundan return yapildiginda calisir */
void signal_return(Trapframe *tf) {
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


SYSCALL_DEFINE2(signal, int, signum, uint32_t, handler) {
	if (signum < 0 || signum > 32 || signum == SIGKILL || signum == SIGSTOP)
		return set_return(tf, -EINVAL);

	//FIXME: handler icin adres kontrolu

	task_curr->signal.action[signum].handler = handler;

	return SYSCALL_RETURN(0);
}
SYSCALL_END(signal)


// FIXME: uygun yere tasi
void copy_stack(uint32_t addr) {
	ASSERT(isRounded(addr));
	memcpy((void*)va2kaddr(addr),
		   (void*)va2kaddr(MMAP_KERNEL_STACK_BASE), MMAP_KERNEL_STACK_SIZE);
#if GCC_VERSION < 40600
	uint32_t *ebp; read_reg(%ebp, ebp);
	for ( ; va2kaddr(MMAP_KERNEL_STACK_TOP) - (uint32_t)ebp < MMAP_KERNEL_STACK_SIZE ;
		 ebp = (uint32_t*)*ebp)
	{
		*(uint32_t*)(va2kaddr(addr) + ((uint32_t)ebp % 0x1000)) = *ebp - MMAP_KERNEL_STACK_OFFSET;
	};
#endif
}

static void push_stack() {
	int r;
	uint32_t eip;

	ASSERT(!task_curr->popped_kstack);
	// print_info(">> push stack to %d\n", task_curr->sigstack.size()+1);
	ASSERT(task_curr->sigstack.size() < 32);

	/* stackin kopyalanacagi page */
	SignalState *s = (SignalState*)kmalloc(sizeof(SignalState));
	s->list_node.init();
	ASSERT(s->list_node.is_free());


	r = task_curr->pgdir.segment_alloc(MMAP_KERNEL_STACK_2_BASE,
									   MMAP_KERNEL_STACK_SIZE,
									   PTE_P | PTE_W);
	ASSERT(r > -1);

	/* yeni olusturulan stackin pageleri signal state icerisine kaydediliyor */
	for (int i = 0 ; i < MMAP_KERNEL_STACK_SIZE/0x1000 ; i++) {
		PTE_t* p = task_curr->pgdir.page_get(MMAP_KERNEL_STACK_2_BASE + i*0x1000);
		ASSERT(p);
		s->stack[i] = p->phys_page();
		s->stack[i]->refcount_inc();
	}

	// FIXME: ebp degerinden stack icin kullanilan offset cikarilmali mi?
	read_reg(%esp, s->esp);
	s->esp -= MMAP_KERNEL_STACK_OFFSET;
	copy_stack(MMAP_KERNEL_STACK_2_BASE);

	/* pop_stack fonksiyonundan buraya atlaniyor */
	eip = read_eip();
	if (eip == 1)
		return;

	s->eip = eip;
	s->sleep = task_curr->sleep;

	ASSERT( s->list_node.is_free() );
	r = task_curr->sigstack.push_front(&s->list_node);
	ASSERT( r );
}

static void pop_stack() {
	int r;
	uint32_t esp, eip;
	SignalState *s;

	ASSERT(!task_curr->popped_kstack);
	ASSERT(task_curr->sigstack.size() > 0);
	// print_info(">> pop stack from %d\n", task_curr->sigstack.size());

	s = task_curr->sigstack.front();
	r = task_curr->sigstack.pop_front();
	ASSERT( r == 1 );

	task_curr->popped_kstack = 1;
	task_curr->sleep = s->sleep;

	for (int i = 0 ; i < MMAP_KERNEL_STACK_SIZE/0x1000 ; i++) {
		r = task_curr->pgdir.page_insert(s->stack[i],
										 MMAP_KERNEL_STACK_2_BASE + i*0x1000,
										 PTE_P | PTE_W);
		ASSERT(r == 2);
		s->stack[i]->refcount_dec();
	}

	esp = s->esp;
	eip = s->eip;

	kfree(s);

	load_reg(%esp, esp);
	asm volatile(
		"push $1\n\t"
		"push %0\n\t"
		"ret\n\t"
		:: "r" (eip));
	while(1);
}

/* task_free_kernel_stack tarafindan kullaniliyor */
void free_sigstack() {
	while (task_curr->sigstack.size() > 0) {
		SignalState *s = task_curr->sigstack.front();
		task_curr->sigstack.pop_front();

		for (int i = 0 ; i < MMAP_KERNEL_STACK_SIZE/0x1000 ; i++) {
			s->stack[i]->refcount_dec();
			ASSERT(s->stack[i]->refcount_get() < 2);
			page_free(s->stack[i]);
		}

		kfree(s);
	}
}
