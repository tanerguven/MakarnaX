#ifndef X86_H_
#define X86_H_

#include <types.h>

/** Protection Enable */
#define CR0_PE		0x00000001
/** Monitor coProcessor */
#define CR0_MP		0x00000002
/** Emulation */
#define CR0_EM		0x00000004
/** Task Switched */
#define CR0_TS		0x00000008
/** Extension Type */
#define CR0_ET		0x00000010
/** Numeric Errror */
#define CR0_NE		0x00000020
/** Write Protect (kernel modda W olmayan sayfalara yazmayi engeller) */
#define CR0_WP		0x00010000
/** Alignment Mask */
#define CR0_AM		0x00040000
/** Not Writethrough */
#define CR0_NW		0x20000000
/** Cache Disable */
#define CR0_CD		0x40000000
/** Paging */
#define CR0_PG		0x80000000

/** Performance counter enable */
#define CR4_PCE		0x00000100
/** Machine Check Enable */
#define CR4_MCE		0x00000040
/** Page Size Extensions */
#define CR4_PSE		0x00000010
/** Debugging Extensions */
#define CR4_DE		0x00000008
/** Time Stamp Disable */
#define CR4_TSD		0x00000004
/** Protected-Mode Virtual Interrupts */
#define CR4_PVI		0x00000002
/** V86 Mode Extensions */
#define CR4_VME	    0x00000001

/* Eflags register */
/** Carry Flag */
#define FL_CF		0x00000001
/** Parity Flag */
#define FL_PF		0x00000004
/** Auxiliary carry Flag */
#define FL_AF		0x00000010
/** Zero Flag */
#define FL_ZF		0x00000040
/** Sign Flag */
#define FL_SF		0x00000080
/** Trap Flag */
#define FL_TF		0x00000100
/** Interrupt Flag */
#define FL_IF		0x00000200
/** Direction Flag */
#define FL_DF		0x00000400
/** Overflow Flag */
#define FL_OF		0x00000800
/** I/O Privilege Level bitmask */
#define FL_IOPL_MASK	0x00003000
/** I/O Privilege Level bitmask, IOPL == 0 */
#define FL_IOPL_0	0x00000000
/** I/O Privilege Level bitmask, IOPL == 1 */
#define FL_IOPL_1	0x00001000
/** I/O Privilege Level bitmask, IOPL == 2 */
#define FL_IOPL_2	0x00002000
/** I/O Privilege Level bitmask, IOPL == 3 */
#define FL_IOPL_3	0x00003000
/** Nested Task */
#define FL_NT		0x00004000
/** Resume Flag */
#define FL_RF		0x00010000
/** Virtual 8086 mode */
#define FL_VM		0x00020000
/** Alignment Check */
#define FL_AC		0x00040000
/** Virtual Interrupt Flag */
#define FL_VIF		0x00080000
/** Virtual Interrupt Pending */
#define FL_VIP		0x00100000
/** ID flag */
#define FL_ID		0x00200000

/* Page fault error codes */
/** Page fault caused by protection violation */
#define FEC_PR		0x1
/** Page fault caused by a write */
#define FEC_WR		0x2
/** Page fault occured while in user mode */
#define FEC_U		0x4


inline uint32_t ebp_read(void) {
	uint32_t ebp;
	asm volatile("movl %%ebp,%0" : "=r" (ebp));
	return ebp;
}

inline void ebp_load(uint32_t ebp) {
	asm volatile ("movl %0, %%ebp\n\t" :: "r" (ebp));
}

inline uint32_t esp_read(void) {
	uint32_t esp;
	asm volatile("movl %%esp,%0" : "=r" (esp));
	return esp;
}

inline void esp_load(uint32_t esp) {
	asm volatile ("movl %0, %%esp\n\t" :: "r" (esp));
}

inline uint32_t ss_read(void) {
	uint32_t ss;
	asm volatile("movl %%ss,%0" : "=r" (ss));
	return ss;
}

inline uint32_t es_read(void) {
	uint32_t es;
	asm volatile("movl %%es,%0" : "=r" (es));
	return es;
}

inline uint32_t ds_read(void) {
	uint32_t ds;
	asm volatile("movl %%ds,%0" : "=r" (ds));
	return ds;
}

inline uint32_t cs_read(void) {
	uint32_t cs;
	asm volatile("movl %%cs,%0" : "=r" (cs));
	return cs;
}

inline void cr3_load(uint32_t pa) {
	asm volatile ("movl %0, %%cr3\n\t" :: "r" (pa));
}

static inline void cr3_reload() {
	asm volatile (""
				  "push %eax\n\t"
				  "mov %cr3, %eax\n\t"
				  "mov %eax, %cr3\n\t"
				  "pop %eax\n\t"
	);
}

inline uint32_t cr3_read() {
	uint32_t cr3;
	asm volatile("movl %%cr3,%0" : "=r" (cr3));
	return cr3;
}

#ifdef __CONF_CPU_invlpg
/** i486 ve sonrasi */
static inline void invlpg(uint32_t addr) {
	asm volatile("invlpg %0" : : "m" (addr) : "memory");
}
#endif

static inline void tlb_invalidate(uint32_t addr) {
#ifdef __CONF_CPU_invlpg
	invlpg(addr);
# else
	cr3_reload();
#endif
}

inline uint32_t cr2_read() {
	uint32_t cr2;
	asm volatile("movl %%cr2,%0" : "=r" (cr2));
	return cr2;
}

static inline void cr0_load(uint32_t val) {
	asm volatile("movl %0,%%cr0" : : "r" (val));
}

static inline uint32_t cr0_read(void) {
	uint32_t val;
	asm volatile("movl %%cr0,%0" : "=r" (val));
	return val;
}

inline void gdt_load(uint32_t p) {
	asm("lgdt (%0)\n\t" :: "r"(p));
}

static inline uint32_t eflags_read() {
	uint32_t eflags;
	asm volatile("pushfl; popl %0" : "=r" (eflags));
	return eflags;
}

static inline void  eflags_load(uint32_t eflags) {
	asm volatile("pushl %0; popfl" :: "r" (eflags));
}

inline void gs_set(uint16_t seg) {
	asm volatile("movw %%ax,%%gs" :: "a" (seg));
}

inline void fs_set(uint16_t seg) {
	asm volatile("movw %%ax,%%fs" :: "a" (seg));
}

inline void es_set(uint16_t seg) {
	asm volatile("movw %%ax,%%es" :: "a" (seg));
}

inline void ds_set(uint16_t seg) {
	asm volatile("movw %%ax,%%ds" :: "a" (seg));
}

inline void ss_set(uint16_t seg) {
	asm volatile("movw %%ax,%%ss" :: "a" (seg));
}

/* segment sabit sayı olması gerektiği için, inline fonksiyonda yazamıyoruz */
#define cs_set(seg)\
	asm volatile("ljmp %0,$cs_set_label \n\t cs_set_label: \n\t":: "i"(seg));


static inline void tr_load(uint16_t sel) {
	asm volatile("ltr %0" : : "r" (sel));
}

static inline void idt_load(uint32_t p) {
	asm volatile("lidt (%0)\n\t" :: "r"(p));
}

extern uint32_t read_eip() __attribute__ ((noinline));

static inline void cli() {
	asm("cli");
}

static inline void sti() {
	asm("sti");
}


/**********************************************
 * bit array operations
 **********************************************/

static inline uint32_t bit_find_first_zero(uint32_t bit_array) {
    uint32_t pos = 0;
    asm("bsfl %1,%0\n\t"
		"jne 1f\n\t"
		"movl $32, %0\n"
		"1:"
		: "=r" (pos)
		: "r" (~(bit_array)));
    if (pos > 31)
        return 32;
    return (uint16_t) pos;
}

static inline uint32_t bit_find_last_zero(uint32_t bit_array) {
    unsigned pos = 0;
    asm("bsrl %1,%0\n\t"
        "jne 1f\n\t"
        "movl $32, %0\n"
        "1:"
		: "=r" (pos)
		: "r" (~(bit_array)));
    if (pos > 31)
        return 32;
    return (uint16_t) pos;
}

static inline void bit_set(uint32_t *bit_array, uint32_t bit) {
    asm("bts %1,%0" : "+m" (*bit_array) : "r" (bit));
}

static inline void bit_reset(uint32_t *bit_array, uint32_t bit) {
    asm("btr %1,%0" : "+m" (*bit_array) : "r" (bit));
}

static inline void bit_complement(uint32_t *bit_array, uint32_t bit) {
    asm("btc %1,%0" : "+m" (*bit_array) : "r" (bit));
}

static inline uint8_t bit_test(uint32_t bit_array, uint32_t bit) {
	/* bit test yap, carry flagi dondur */
	uint8_t x;
	asm("bt %2, %1\n\t"
		"lahf\n\t"
		"mov %%ah, %0\n\t"
		: "=r" (x)
		: "r" (bit_array), "r" (bit));
	return x & FL_CF;
}

#endif /* X86_H_ */
