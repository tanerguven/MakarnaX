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

#include "kernel.h"

#include <types.h>

#include <kernel/syscall.h>

#include "memory/virtual.h"
#include "task.h"
#include "sched.h"
#include <errno.h>
#include "kernel.h"

// console.cpp
extern TaskList_t console_input_list;
extern int console_getc();

// syscall_table.c
extern void (* const syscalls[])();


SYSCALL_DEFINE0(nosys) {
	printf(">> no sys %d\n", tf->regs.eax);

	return SYSCALL_RETURN(-ENOSYS);
}
SYSCALL_END(nosys)


asmlink void do_syscall(uint32_t no) {
	if (no < MAX_SYSCALL_COUNT) {
		syscalls[no]();
		return;
	}

	sys_nosys();
}


SYSCALL_DEFINE2(cputs, const char*, s, size_t, len) {
	/* adresin kullaniciya ait oldugundan ve dogru oldugundan emin ol */
	if ( task_curr->pgdir.verify_user_addr(s, len, PTE_U) < 0 ) {
		printf(">> cputs not verified: 0x%08x - 0x%08x\n", s, s+len);
		do_exit(111);
	}

	s = (const char*)uaddr2kaddr((uint32_t)s);

	for (uint32_t i = 0 ; (i < len) && (*s != '\0') ; i++, s++) {
		putchar(*s);
	}
}
SYSCALL_END(cputs)


SYSCALL_DEFINE0(cgetc) {
	sleep_interruptible(&console_input_list);

	return SYSCALL_RETURN(console_getc());
}
SYSCALL_END(cgetc)


SYSCALL_DEFINE0(dongu) {
	for (uint32_t i = 0 ;  i < 0XFFFF; i++) {
		asm volatile("nop");
	}
	return SYSCALL_RETURN(-1);
}
SYSCALL_END(dongu)


/**
 * @return end_brk: normal, 0: hata
 */
SYSCALL_DEFINE1(brk, uint32_t, brk) {
	int r;
	uint32_t i;

	uint32_t eflags = eflags_read();
	cli();

	if (brk < task_curr->pgdir.start_brk) {
		eflags_load(eflags);
		return SYSCALL_RETURN(task_curr->pgdir.end_brk);
	}

	if (brk > task_curr->pgdir.end_brk) {
		for (i = roundUp(task_curr->pgdir.end_brk) ; i < roundUp(brk) ; i += 0x1000) {
			r = task_curr->pgdir.page_alloc_insert(uaddr2va(i), PTE_P | PTE_U | PTE_W);
			if (r < 0)
				goto bad_sys_brk_alloc;
		}
	} else {
		for (i = roundUp(task_curr->pgdir.end_brk) - 0x1000 ; i >= roundUp(brk) ; i -= 0x1000) {
			r = task_curr->pgdir.page_remove(uaddr2va(i), 1);
			/* heap icerisinde map edilmemis bir page olamaz */
			ASSERT(r > -1);
		}
	}

	task_curr->pgdir.end_brk = brk;

	eflags_load(eflags);
	return SYSCALL_RETURN(task_curr->pgdir.end_brk);

bad_sys_brk_alloc:
	for ( ; i >= task_curr->pgdir.end_brk ; i -= 0x1000) {
		task_curr->pgdir.page_remove(uaddr2va(i), 1);
	}
bad_sys_brk:
	return SYSCALL_RETURN(0);
}
SYSCALL_END(brk)


uint32_t user_to_kernel_check(uint32_t base, uint32_t limit, int rw) {
	int perm = (rw) ? PTE_W | PTE_U : PTE_U;
	if ( task_curr->pgdir.verify_user_addr((const void*)base, limit, perm) < 0 ) {
		printf(">> user addr not verified: 0x%08x - 0x%08x\n", base,
			   base+limit);
		do_exit(111);
	}

	return uaddr2kaddr(base);
}
