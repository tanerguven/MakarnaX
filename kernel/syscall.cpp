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

#include "panic.h"

#include <types.h>
#include <stdio.h>

#include <kernel/syscall.h>

#include "memory/virtual.h"
#include "task.h"
#include "sched.h"
#include <errno.h>

//
asmlink void do_exit(int);
//

// console.cpp
extern TaskList_t console_input_list;
//

/* */
asmlink void sys_exit();
asmlink void sys_fork();
asmlink void sys_getpid();
asmlink void sys_alarm();
asmlink void sys_pause();
asmlink void sys_kill();
asmlink void sys_signal();
asmlink void sys_ipc();
asmlink void sys_brk();


asmlink void sys_cputs();
asmlink void sys_cgetc();
asmlink void sys_yield();
asmlink void sys_wait();
asmlink void sys_dongu();
asmlink void sys_sleep();
asmlink void sys_sbrk();

/* */

void sys_nosys() {
	Trapframe *tf = task_curr->registers();
	printf("sys %d\n", tf->regs.eax);
	PANIC("nosys");

	return set_return(tf, -ENOSYS);
}

void (*syscalls[])() = {
	sys_nosys, /* 0 */
	sys_exit, sys_fork, sys_nosys, sys_nosys, sys_nosys, /* 5 */
	sys_nosys, sys_nosys, sys_nosys, sys_nosys, sys_nosys,
	sys_nosys, sys_nosys, sys_nosys, sys_nosys, sys_nosys,
	sys_nosys, sys_nosys, sys_nosys, sys_nosys, sys_getpid,
	sys_nosys, sys_nosys, sys_nosys, sys_nosys, sys_nosys,

	sys_nosys, sys_alarm, sys_nosys, sys_pause, sys_nosys,
	sys_nosys, sys_nosys, sys_nosys, sys_nosys, sys_nosys,
	sys_nosys, sys_kill, sys_nosys, sys_nosys, sys_nosys,
	sys_nosys, sys_nosys, sys_nosys, sys_nosys, sys_brk,
	sys_nosys, sys_nosys, sys_signal, sys_nosys, sys_nosys, /* 50 */

	sys_nosys, sys_nosys, sys_nosys, sys_nosys, sys_nosys,
	sys_nosys, sys_nosys, sys_nosys, sys_nosys, sys_nosys,
	sys_nosys, sys_nosys, sys_nosys, sys_nosys, sys_nosys,
	sys_nosys, sys_nosys, sys_nosys, sys_nosys, sys_nosys,
	sys_nosys, sys_nosys, sys_nosys, sys_nosys, sys_nosys, /* 75 */

	sys_nosys, sys_nosys, sys_nosys, sys_nosys, sys_nosys,
	sys_nosys, sys_nosys, sys_nosys, sys_nosys, sys_nosys,
	sys_nosys, sys_nosys, sys_nosys, sys_nosys, sys_nosys,
	sys_nosys, sys_nosys, sys_nosys, sys_nosys, sys_nosys,
	sys_nosys, sys_nosys, sys_nosys, sys_nosys, sys_nosys, /* 100 */

	sys_nosys, sys_nosys, sys_nosys, sys_nosys, sys_nosys,
	sys_nosys, sys_nosys, sys_nosys, sys_nosys, sys_nosys,
	sys_nosys, sys_nosys, sys_nosys, sys_nosys, sys_nosys,
	sys_nosys, sys_ipc, sys_nosys, sys_nosys, sys_nosys,
	sys_nosys, sys_nosys, sys_nosys, sys_nosys, sys_nosys,
	sys_nosys, sys_nosys, sys_nosys, sys_nosys, sys_nosys, /* 125 */

	sys_nosys,
};

void (*syscalls_2[])() = {
	sys_nosys, /* 1000 */
	sys_cputs, sys_cgetc, sys_yield, sys_wait, sys_dongu, /* 1005 */
	sys_sleep, sys_sbrk
};

asmlink void do_syscall(uint32_t no) {
	if (no < sizeof(syscalls)) {
		syscalls[no]();
		return;
	}

	if (no > 1000 && no < (1000 + sizeof(syscalls_2))) {
		syscalls_2[no-1000]();
		return;
	}

	sys_nosys();
}

asmlink void sys_cputs() {
	Trapframe* tf = task_curr->registers();
	const char *s = (const char*)get_param1(tf);
	size_t len = (size_t)get_param2(tf);

	/* adresin kullaniciya ait oldugundan ve dogru oldugundan emin ol */
	if ( task_curr->pgdir.verify_user_addr(s, len, PTE_U) < 0 ) {
		printf(">> sys_cputs not verified: 0x%08x - 0x%08x\n", s, s+len);
		do_exit(111);
	}

	s = (const char*)uaddr2kaddr((uint32_t)s);

	for (uint32_t i = 0 ; (i < len) && (*s != '\0') ; i++, s++) {
		putchar(*s);
	}
}

int console_getc();

asmlink void sys_cgetc() {
	// FIXME: registerlar kaydedilmediginde run-keyboard'da page fault aliniyor
	if (!task_curr->registers_saved)
		task_curr->save_new_registers();

	sleep_interruptible(&console_input_list);

	return set_return(task_curr->registers(), console_getc());
}

asmlink void sys_dongu() {
	for (uint32_t i = 0 ;  i < 0XFFFF; i++) {
		asm volatile("nop");
	}
}


asmlink void sys_brk() {
	ASSERT(isRounded(task_curr->pgdir.start_brk));

	int r;
	uint32_t i;

	Trapframe *tf = task_curr->registers();
	uint32_t brk = get_param1(tf);

	uint32_t eflags = eflags_read();
	cli();

	if (brk < task_curr->pgdir.start_brk) {
		eflags_load(eflags);
		return set_return(tf, -1);
	}

	brk = roundUp(brk);

	if (brk > task_curr->pgdir.end_brk) {
		for (i = task_curr->pgdir.end_brk ; i < brk ; i += 0x1000) {
			r = task_curr->pgdir.page_alloc_insert(uaddr2va(i), PTE_P | PTE_U | PTE_W);
			if (r < 0)
				goto bad_sys_brk_alloc;
			// printf(">> insert %08x\n", uaddr2va(i));
		}
	} else {
		for (i = task_curr->pgdir.end_brk - 0x1000 ; i >= brk ; i -= 0x1000) {
			r = task_curr->pgdir.page_remove(uaddr2va(i), 1);
			if (r < 0) {
				printf(">> sys_brk bug olabilir\n");
				goto bad_sys_brk;
			}
			// printf(">> remove %08x\n", uaddr2va(i));
		}
	}

	task_curr->pgdir.end_brk = brk;

	ASSERT(isRounded(task_curr->pgdir.end_brk));
	eflags_load(eflags);

	return set_return(tf, 0);

bad_sys_brk_alloc:
	for ( ; i >= task_curr->pgdir.end_brk ; i -= 0x1000) {
		task_curr->pgdir.page_remove(uaddr2va(i), 1);
	}
bad_sys_brk:
	return set_return(tf, -1);
}

asmlink void sys_sbrk() {
	Trapframe *tf = task_curr->registers();
	uint32_t increment = get_param1(tf);

	uint32_t eflags = eflags_read();
	cli();

	if (increment == 0) {
		/* sbrk(0), end_brk dondurur */
		eflags_load(eflags);
		return set_return(tf, task_curr->pgdir.end_brk);
	}

	printf(">> sys_sbrk tamamlanmadi\n");

	eflags_load(eflags);
	return set_return(tf, -1);
}
