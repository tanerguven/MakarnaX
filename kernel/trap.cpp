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
 * exception, interrupt ve sistem cagrilarini yakalamak icin islemler.
 *
 * exception: bolme hatasi, page fault gibi programin isleyisinden kaynaklanan
 * durumlar.
 * interrupt: driverlardan kaynaklanan kesmeler vb. durumlar.
 * trap: exception, interrupt ya da sistem cagirisindan herhangi biri anlaminda
 *
 */

#include <kernel/kernel.h>

#include "trap.h"
#include "task.h"
#include <asm/x86.h>

#include "sched.h"
#include "signal.h"

/* traphandler.S dosyasinda tanimli fonksiyonlar */
asmlink void divide_error(void);
asmlink void debug();
asmlink void nmi();
asmlink void int3();
asmlink void overflow(); /* 5 */
asmlink void bounds_check();
asmlink void invalid_opcode();
asmlink void device_not_available();
asmlink void double_fault();
asmlink void coprecosser_segment_overrun(); /* 10 */
asmlink void invalid_TSS();
asmlink void segment_not_present();
asmlink void stack_exception();
asmlink void general_protection();
asmlink void page_fault(); /* 15 */
asmlink void reserved();
asmlink void floating_point_error();
asmlink void alignment_check();
asmlink void machine_check();
asmlink void SIMD_floating_point_error(); /* 20 */
asmlink void irq_timer(); /* IRQ_OFFSET+IRQ_TIMER = 32 */
asmlink void irq_keyboard(); /* IRQ_OFFSET+IRQ_KBD = 33 */
asmlink void syscall_trap(); /* 0x80 */
/* */

// memory.cpp
extern SegmentDesc gdt[];

// sched.cpp
asmlink void do_timer(Trapframe*);

// keyboard.cpp
asmlink void do_irq_keyboard(Trapframe*);

// syscall.cpp
asmlink void do_syscall(int no);

// kernel_monitor.cpp
extern bool kernel_monitor_running;

// fonksiyon prototipleri
void do_error(Trapframe*);
void do_unknown(Trapframe*);
void do_int3(Trapframe*);
void do_page_fault(Trapframe*);
//

struct TrapFunction {
	const char *name;
	void (*fn)(Trapframe*);
};

/** exceptionlar icin kullanilacak fonksiyonlar */
static const TrapFunction exceptions[] = {
	{ "Divide error", do_error },
	{ "Debug", do_error },
	{ "Non-Maskable Interrupt", do_error },
	{ "Breakpoint", do_error },
	{ "Overflow", do_error },
	{ "BOUND Range Exceeded", do_error },
	{ "Invalid Opcode", do_error },
	{ "Device Not Available", do_error },
	{ "Double Fault", do_error },
	{ "Coprocessor Segment Overrun", do_error },
	{ "Invalid TSS", do_error },
	{ "Segment Not Present", do_error },
	{ "Stack Fault", do_error },
	{ "General Protection", do_error },
	{ "Page Fault", do_page_fault },
	{ "unknown exception", do_unknown },
	{ "x87 FPU Floating-Point Error", do_error },
	{ "Alignment Check", do_error },
	{ "Machine-Check", do_error },
	{ "SIMD Floating-Point Exception", do_error }
};

static const TrapFunction irq_handlers[] = {
	{ "IRQ Timer", do_timer },
	{ "IRQ Keyboard", do_irq_keyboard },
	{ "-", do_unknown },
	{ "-", do_unknown },
	{ "-", do_unknown },
	{ "-", do_unknown },
	{ "-", do_unknown },
	{ "-", do_unknown }, /* 10 */
	{ "-", do_unknown },
	{ "-", do_unknown },
	{ "-", do_unknown },
	{ "-", do_unknown },
	{ "-", do_unknown }, /* 16 */
};

static struct TSS tss;
GateDesc idt[256] = { { 0 } };
PseudoDesc idt_pd;

void init_traps() {

	memset(idt, 0, sizeof(idt));
	idt_pd.base = kaddr2va((uint32_t)idt);
	idt_pd.lim = sizeof(idt) - 1;

	/*
	 * traphandler.S dosyasinda tanimlanmis fonksiyonlari idtye trap
	 * numaralarina ve trap tipine gore ekliyoruz.
	 *
	 * divide error oldugunda divide_error
	 * ...
	 * sistem cagirisi oldugunda syscall_trap fonksiyonuna atlanacak
	 */
	idt[T_DIVIDE].set_trap_gate(divide_error);
	idt[T_DEBUG].set_trap_gate(debug);
	idt[T_NMI].set_trap_gate(nmi);
	idt[T_BRKPT].set_system_gate(int3);
	idt[T_OFLOW].set_system_gate(overflow);
	idt[T_BOUND].set_system_gate(bounds_check);
	idt[T_ILLOP].set_trap_gate(invalid_opcode);
	idt[T_DEVICE].set_trap_gate(device_not_available);
	idt[T_DBLFLT].set_trap_gate(double_fault);
 	idt[T_COPROC].set_trap_gate(coprecosser_segment_overrun);
 	idt[T_TSS].set_trap_gate(invalid_TSS);
 	idt[T_SEGNP].set_trap_gate(segment_not_present);
 	idt[T_STACK].set_trap_gate(stack_exception);
 	idt[T_GPFLT].set_trap_gate(general_protection);
	idt[T_PGFLT].set_trap_gate(page_fault);
	idt[T_RES].set_trap_gate(reserved);
	idt[T_FPERR].set_trap_gate(floating_point_error);
	idt[T_ALIGN].set_trap_gate(alignment_check);
	idt[T_MCHK].set_trap_gate(machine_check);
	idt[T_SIMDERR].set_trap_gate(SIMD_floating_point_error);
	for (int i = 21 ; i < 32 ; i++) {
		idt[i].set_trap_gate(reserved);
	}
	idt[IRQ_OFFSET+IRQ_TIMER].set_int_gate(irq_timer);
	idt[IRQ_OFFSET+IRQ_KBD].set_int_gate(irq_keyboard);
	idt[T_SYSCALL].set_system_gate(syscall_trap);

	/* trap oldugunda kullanilacak kernel stack ve code segment */
	tss.esp0 = va2kaddr(MMAP_KERNEL_STACK_TOP);
	tss.ss0 = GD_KD;

	/* gdt'nin tss alani */
	uint32_t tss_ptr = kaddr2va((uint32_t)&tss);
	gdt[GD_TSS >> 3].set_seg16(STS_T32A, tss_ptr,
					sizeof(struct TSS), 0);
	gdt[GD_TSS >> 3].s = 0;

	/* load TSS */
	tr_load(GD_TSS);

	/* load IDT */
	idt_load((uint32_t)&idt_pd);

	print_info(">> init_traps OK\n");
}

/** kayitli olmayan exception durumu */
static const TrapFunction exception_unknown = {
	"unknown exception", do_error
};

void print_trapframe(Trapframe *tf) {

	print_error("err: %08x\n"
				"eip: %08x\n"
				"cs: %08x\n"
				"ds: %08x\n"
				"esp: %08x\n"
				"ebp: %08x\n",
				tf->err, tf->eip, tf->cs, tf->ds, tf->esp, tf->regs.ebp);

	print_error("eax: %08x\n"
				"ecx: %08x\n"
				"edx: %08x\n"
				"ebx: %08x\n"
				"edi: %08x\n"
				"esi: %08x\n",
				tf->regs.eax, tf->regs.ecx, tf->regs.edx, tf->regs.ebx,
				tf->regs.edi, tf->regs.esi);
}

asmlink void trap_handler(Trapframe *tf) {
	/*
	 * fonksiyondan normal bicimde return yapildiginda current task calismaya
	 * devam eder. (traphandler.S, trapret)
	 */

	const bool user_mode_trap = (tf->cs & 3) == 3;

	ASSERT_DTEST(task_curr);

	task_curr->popped_kstack = 0;

	if (tf->trapno >= IRQ_OFFSET && tf->trapno < 48) {
		/* kernel monitor calisiyorsa, donanim kesmeleri devre disi */
		if (kernel_monitor_running)
			return;

		ASSERT_int_disable();

		irq_handlers[tf->trapno-IRQ_OFFSET].fn(tf);
/*
* user modda donanim kesmesi olduysa task switch yapilabilir (timer).
* taskin signal durumlarindan dolayi return_trap_handler ile devam edilmeli
*/
		if (user_mode_trap)
			goto return_trap_handler;

		/* kernel moddaki donanim kesmelerinde kaldigi yerden devam eder */
		return;
	}

	/* fonksiyonun devamina kernel mod traplarindan ulasilmamali */
	if (!user_mode_trap) {
		print_error("trapno: %d\n", tf->trapno);
		print_trapframe(tf);
		uint32_t cr2; read_reg(%cr2, cr2);
		print_error("cr2: %08x\n", cr2);
		PANIC("kernel mode trap");
		return;
	}

	/* user mode */
	if (tf->trapno == T_SYSCALL) {
		do_syscall(tf->regs.eax);
		goto return_trap_handler;
	}

	ASSERT_int_disable();

	if (tf->trapno < sizeof(exceptions)/sizeof(exceptions[0]))
		exceptions[tf->trapno].fn(tf);
	else
		do_unknown(tf);

return_trap_handler:
	// cli(); // interruptlar disable olmazsa tuhaf hatalar oluyor
	ASSERT(task_curr->state == Task::State_running);
}

void do_error(Trapframe* tf) {
	ASSERT_int_disable();

	const TrapFunction *e = &exception_unknown;
	if (tf->trapno < sizeof(exceptions)/sizeof(exceptions[0]))
		e = &exceptions[tf->trapno];
	print_error("%s\n", e->name);
	print_trapframe(tf);
	do_exit(111);
}

void do_unknown(Trapframe* tf) {
	ASSERT_int_disable();

	if (tf->trapno >= IRQ_OFFSET && tf->trapno < 48) {
		print_warning(">> unknown IRQ %d\n", tf->trapno);
		return;
	}

	print_trapframe(tf);
	if ((tf->cs & 3) == 0) {
		PANIC(">> unknown trap in kernel");
	} else {
		print_error(">> trapno: %d\n", tf->trapno);
		PANIC(">> unknown trap in user - (destroy task)");
	}
}

void do_page_fault(Trapframe *tf) {
	ASSERT_int_disable();

	/* bu fonksiyon kernel mode page faultta calismaz */
	ASSERT_DTEST((tf->cs & 3) == 3);

	int r;
	uint32_t fault_va; read_reg(%cr2, fault_va);

	/* programin sonlanmasi durumu */
	if (tf->eip == va2uaddr(0xffffffff)) {
		print_info(">> program normal bir bicimde sonlandi. [%d]\n",
			   task_curr->id);
		print_info(">> return degeri: %08x\n", tf->regs.eax);
		do_exit(0);
	}

	/* signal handle fonksiyonunun tamamlanmasi durumu */
	if (tf->eip == va2uaddr(0xfffffffe) && task_curr->signal.pending) {
		signal_return(tf);
		return;
	}

	/* stackin buyumesi durumu */
	uint32_t stack_base = MMAP_USER_STACK_TOP - task_curr->pgdir.count_stack * 0x1000;
	if (stack_base - fault_va < 0x1000) {
		/* stackin bir altinda page'de fault olduysa */

		/* stack limiti asilmissa processi sonlandir */
		if (fault_va < MMAP_USER_STACK_BASE_LIMIT) {
			print_warning(">> stack limiti asildi\n"
				   "\tfault_va: %08x\n"
				   "\tstack_base_limit: %08x\n",
				   fault_va,
				   MMAP_USER_STACK_BASE_LIMIT);
			do_exit(111);
			return;
		}

		/* stack limiti asilmamissa page insert yap */
		stack_base -= 0x1000;
		r = task_curr->pgdir.page_alloc_insert(stack_base, PTE_P | PTE_U | PTE_W);
		if (r < 0) {
			/* bellek yetmediyse processi sonlandir */
			print_warning(">> stacki buyutmek icin bellek yetmedi\n");
			do_exit(222);
			return;
		}
		task_curr->pgdir.count_stack++;
		return;
	}

	print_trapframe(tf);
	print_warning(">> fault va: %08x\n", fault_va);

	/* user modda bilinmeyen bir page fault olursa programi sonlandir */
	do_exit(111);
}
