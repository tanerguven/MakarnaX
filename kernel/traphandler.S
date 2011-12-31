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

/*
 * exception, interrupt ve sistem cagrilarinin yakalanmasi icin alt seviye
 * islemler.
 */

#include <kernel/trap.h>

#define TRAPHANDLER(name, num) \
		.globl name				;\
		.type name, @function	;\
		.align 2				;\
name:	\
		pushl $(num)			;\
		jmp _alltraps

#define TRAPHANDLER_NO_ERROR_CODE(name, num) \
		.globl name				;\
		.type name, @function	;\
		.align 2				;\
name:	\
		pushl $0 /* error code*/;\
		pushl $(num)			;\
		jmp _alltraps


TRAPHANDLER_NO_ERROR_CODE(divide_error, T_DIVIDE)
TRAPHANDLER_NO_ERROR_CODE(debug, T_DEBUG)
TRAPHANDLER_NO_ERROR_CODE(nmi, T_NMI)
TRAPHANDLER_NO_ERROR_CODE(int3, T_BRKPT)
TRAPHANDLER_NO_ERROR_CODE(overflow, T_OFLOW)

TRAPHANDLER_NO_ERROR_CODE(bounds_check, T_BOUND)
TRAPHANDLER_NO_ERROR_CODE(invalid_opcode, T_ILLOP)
TRAPHANDLER_NO_ERROR_CODE(device_not_available, T_DEVICE)
TRAPHANDLER(double_fault, T_DBLFLT)
TRAPHANDLER(coprecosser_segment_overrun, T_COPROC)

TRAPHANDLER(invalid_TSS, T_TSS)
TRAPHANDLER(segment_not_present, T_SEGNP)
TRAPHANDLER(stack_exception, T_STACK)
TRAPHANDLER(general_protection, T_GPFLT)
TRAPHANDLER(page_fault, T_PGFLT)

TRAPHANDLER(reserved, T_RES)
TRAPHANDLER_NO_ERROR_CODE(floating_point_error, T_FPERR)
TRAPHANDLER(alignment_check, T_ALIGN)
TRAPHANDLER_NO_ERROR_CODE(machine_check, T_MCHK)
TRAPHANDLER_NO_ERROR_CODE(SIMD_floating_point_error, T_SIMDERR)
/* 21 - 31 reserved */
TRAPHANDLER_NO_ERROR_CODE(syscall_trap, T_SYSCALL)
TRAPHANDLER_NO_ERROR_CODE(irq_timer, IRQ_OFFSET+IRQ_TIMER)
TRAPHANDLER_NO_ERROR_CODE(irq_keyboard, IRQ_OFFSET+IRQ_KBD)

_alltraps:
		/* Trapframe olustur */
		pushl %ds
		pushl %es
		pushal
		/* Load kernel descriptors */
		movw $GD_KD, %ax
		movw %ax, %ds
		movw %ax, %es
		/* fonksiyona stack pointerini parametre olarak ekle */
		push %esp
		/* "trap.c"deki trap_handler fonksiyonunu cagir */
		call trap_handler
		/* pop pushed stack pointer */
		addl $4, %esp
.globl trapret
trapret:
		popal
		pop %es
		pop %ds
		/* trapno ve error kodunu atla */
		addl $8, %esp
		iret
