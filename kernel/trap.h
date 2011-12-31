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
 * segment, tss, trapframe veriyapilari
 */

#ifndef TRAP_H_
#define TRAP_H_

//#ifndef __ASSEMBLER__
#include <types.h>
#include <kernel/trap.h>

/**
 * Intel 80386 Reference Programmer's Manual, 5.1, Figure 5-3
 * http://pdos.csail.mit.edu/6.828/2011/readings/i386/s05_01.htm#fig5-3
 */
struct SegmentDesc {
	inline SegmentDesc() {}

	inline SegmentDesc(uint32_t type, uint32_t base, uint32_t lim, uint32_t dpl,
					   uint32_t g) :
		lim_15_0(lim & 0xffff),
		base_15_0(base & 0xffff),
		base_23_16((base>>16) & 0xff),
		type(type),
		s(1),
		dpl(dpl),
		p(1),
		lim_19_16((lim>>16) & 0xff),
		avl(0),
		__21(0),
		x(1),
		g(g),
		base_31_24(base>>24) {}

	inline void set_seg(uint32_t type, uint32_t base, uint32_t lim, uint32_t dpl) {
		*this = SegmentDesc(type, base, lim>>12, dpl, 1);
	}

	inline void set_seg16(uint32_t type, uint32_t base, uint32_t lim, uint32_t dpl) {
		*this = SegmentDesc(type, base, lim, dpl, 0);
	}

	inline void set_segNull() {
		((uint32_t*)(this))[0] = 0;
		((uint32_t*)(this))[1] = 0;
	}

	uint32_t lim_15_0 : 16;
	uint32_t base_15_0 : 16;
	uint32_t base_23_16 : 8;
	uint32_t type : 4;
	/** 0 = system, 1 = application */
	uint32_t s : 1;
	/** Descriptor Privilege Level */
	uint32_t dpl : 2;
	/** Present */
	uint32_t p : 1;
	uint32_t lim_19_16 : 4;
	/** Unused (available for software use) */
	uint32_t avl : 1;
	/** Reserved */
	uint32_t __21 : 1;
	/** 0 = 16-bit segment, 1 = 32-bit segment */
	uint32_t x : 1;
	/** Granularity: limit scaled by 4K when set */
	uint32_t g : 1;
	uint32_t base_31_24 : 8;
};


/** http://pdos.csail.mit.edu/6.828/2011/readings/i386/s09_04.htm#fig9-2 */
struct PseudoDesc {
	uint16_t lim;
	uint32_t base;
} __attribute__((packed));


/** http://pdos.csail.mit.edu/6.828/2011/readings/i386/s09_05.htm */
struct GateDesc {
	inline void set_gate(uint32_t type, uint32_t sel, uint32_t off, uint32_t dpl) {
		off_15_0 = off & 0xffff;
		seg_selector = sel;
		__37_33 = 0;
		__40_38 = 0;
		this->type = type;
		s = 0;
		this->dpl = dpl;
		p = 1;
		off_31_16 = (off >> 16) & 0xffff;
	}

	/*
	 * TODO: gate tipleri duzenlenmeli
	 */
	/** exceptions */
	inline void set_trap_gate(void (*off)()) {
		/* set_gate(STS_TG32, GD_KT, (uint32_t)off, 0); */
		set_gate(STS_IG32, GD_KT, (uint32_t)off, 0);
	}

	/** User ve kernel moddan cagirilabilen traplar (system call) */
	inline void set_system_gate(void (*off)()) {
		/* set_gate(STS_IG32, GD_KT, (uint32_t)off, 3); */
		set_gate(STS_TG32, GD_KT, (uint32_t)off, 3);
	}

	/** interrupt gate */
	inline void set_int_gate(void (*off)()) {
		set_gate(STS_IG32, GD_KT, (uint32_t)off, 0);
	}

	uint32_t off_15_0 : 16;
	/** segment selector */
	uint32_t seg_selector : 16;
	uint32_t __37_33 : 5;
	/** reserved, 0 */
	uint32_t __40_38 : 3;
	uint32_t type : 4;
	/** 0 */
	uint32_t s : 1;
	uint32_t dpl : 2;
	/** present */
	uint32_t p : 1;
	uint32_t off_31_16 : 16;
};

/* jos : inc/trap.h */
struct PushRegs {
	/* pusha ile stacke basilan registerlar */
	uint32_t edi;
	uint32_t esi;
	uint32_t ebp;
	uint32_t oesp;		/* kullanilmayan */
	uint32_t ebx;
	uint32_t edx;
	uint32_t ecx;
	uint32_t eax;
} __attribute__((packed));

struct Trapframe {
	struct PushRegs regs;
	uint16_t es;
	uint16_t padding1;
	uint16_t ds;
	uint16_t padding2;
	uint32_t trapno;

	uint32_t err;
	uint32_t eip;
	uint16_t cs;
	uint16_t padding3;
	uint32_t eflags;
	/* asagidakiler sadece kernel ve user arasi gecislerde kullaniliyor */
	uint32_t esp;
	uint16_t ss;
	uint16_t padding4;
} __attribute__((packed));


/* sistem cagrilarinda parametreleri karistirmadan okumak icin */
inline uint32_t get_param1(Trapframe* tf) {
	return tf->regs.edx;
}
inline uint32_t get_param2(Trapframe* tf) {
	return tf->regs.ecx;
}
inline uint32_t get_param3(Trapframe* tf) {
	return tf->regs.ebx;
}
inline uint32_t get_param4(Trapframe* tf) {
	return tf->regs.edi;
}
inline uint32_t get_param5(Trapframe* tf) {
	return tf->regs.esi;
}
inline void set_return(Trapframe* tf, uint32_t ret) {
	tf->regs.eax = ret;
}
/* */

struct TSS {
	uint32_t back_link;
	uint32_t esp0;
	uint32_t ss0; /* 16 high bits zero */
	uint32_t esp1;
	uint32_t ss1; /* 16 high bits zero */
	uint32_t esp2;
	uint32_t ss2; /* 16 high bits zero */
	uint32_t cr3;
	uint32_t eip;
	uint32_t eflags;
	uint32_t eax;
	uint32_t ecx;
	uint32_t edx;
	uint32_t ebx;
	uint32_t esp;
	uint32_t ebp;
	uint32_t esi;
	uint32_t edi;
	uint32_t es; /* 16 high bits zero */
	uint32_t cs; /* 16 high bits zero */
	uint32_t ss; /* 16 high bits zero */
	uint32_t ds; /* 16 high bits zero */
	uint32_t fs; /* 16 high bits zero */
	uint32_t gs; /* 16 high bits zero */
	uint32_t ldt; /* 16 high bits zero */
	uint16_t t; /* 15 high bits zero */
	uint16_t io_map_base;
};

inline void push_registers(PushRegs *dest) {
	asm volatile("pushal");
	asm("movl (%%esp), %0" : "=r" (dest->edi));
	asm("movl 4(%%esp), %0" : "=r" (dest->esi));
	asm("movl 8(%%esp), %0" : "=r" (dest->ebp));
	asm("movl 12(%%esp), %0" : "=r" (dest->oesp));
	asm("movl 16(%%esp), %0" : "=r" (dest->ebx));
	asm("movl 20(%%esp), %0" : "=r" (dest->edx));
	asm("movl 24(%%esp), %0" : "=r" (dest->ecx));
	asm("movl 28(%%esp), %0" : "=r" (dest->eax));
	asm volatile("popal");
}

/* #endif /\* __ASSEMBLER__ *\/ */

#endif /* TRAP_H_ */
