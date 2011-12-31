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
 * Paging ile ilgili veriyapilarini icerir.
 */

#ifndef _MEMORY_VIRTUAL_H_
#define _MEMORY_VIRTUAL_H_

#include "../panic.h"
#include <asm/x86.h>
#include <types.h>
#include <string.h>
#include <errno.h>
#include "physical.h"

/*
 * Kernel ve user adres uzaylarinin yerleri bu makrolar ile degistirilebilir
 */
#if 0
/* segmantasyon acki, son 1 gb kernel */
#define MMAP_KERNEL_BASE 0xC0000000
#define MMAP_KERNEL_STACK_TOP 0xF0000000
#define MMAP_KERNEL_TMP_PAGE_BASE 0xD0000000
#define MMAP_USER_BASE 0x00000000
#define MMAP_USER_STACK_TOP 0xC0000000
#endif

#if 1
/* segmantasyon acik, ilk 1 gb kernel */
#define MMAP_KERNEL_BASE 0x00000000
#define MMAP_KERNEL_STACK_TOP 0x40000000
#define MMAP_KERNEL_TMP_PAGE_BASE 0x20000000
#define MMAP_USER_BASE 0x40000000
#define MMAP_USER_STACK_TOP 0xF0000000
#endif

#if 0
/* segmantasyon kapali, ilk 16 mb kernel alani */
#define MMAP_KERNEL_BASE 0x00000000
#define MMAP_KERNEL_STACK_TOP 0x01000000
#define MMAP_KERNEL_TMP_PAGE_BASE 0x00800000
#define MMAP_USER_BASE 0x01000000
#define MMAP_USER_STACK_TOP 0xF0000000
#define MMAP_SEG_KERNEL_BASE 0
#define MMAP_SEG_USER_BASE 0
#endif

#define MMAP_KERNEL_STACK_BASE (MMAP_KERNEL_STACK_TOP - 0x1000)
#define MMAP_KERNEL_TOP MMAP_KERNEL_STACK_TOP
#define MMAP_USER_LIMIT (MMAP_USER_TOP-MMAP_USER_BASE)
#define MMAP_USER_TOP MMAP_USER_STACK_TOP
#define MMAP_USER_SHARED_MEM_BASE ((uint32_t)MMAP_USER_BASE + 0x60000000)
#define MMAP_USER_SHARED_MEM_TOP ((uint32_t)MMAP_USER_BASE + 0x70000000)
#define MMAP_USER_STACK_LIMIT (0x00800000-0x1000)
#define MMAP_USER_STACK_BASE_LIMIT (MMAP_USER_STACK_TOP - MMAP_USER_STACK_LIMIT)

/* segmantasyon aciksa segmantasyon makrolarina adresleri ataniyor */
#ifndef MMAP_SEG_KERNEL_BASE
# define MMAP_SEG_KERNEL_BASE MMAP_KERNEL_BASE
#endif
#ifndef MMAP_SEG_USER_BASE
# define MMAP_SEG_USER_BASE MMAP_USER_BASE
#endif

#if MMAP_KERNEL_BASE == MMAP_USER_BASE
# error kernel base ve user base esit olamaz
#endif
#if (MMAP_USER_BASE % 0x01000000) != 0
# error MMAP_USER_BASE 4 mb kati olmali
#endif
#if (MMAP_KERNEL_BASE % 0x01000000) != 0
# error MMAP_KERNEL_BASE 4 mb kati olmali
#endif

/* Present */
#define PTE_P		0x001
/* Writeable */
#define PTE_W		0x002
/* User */
#define PTE_U		0x004
/* Write-Through */
#define PTE_PWT		0x008
/* Cache-Disable */
#define PTE_PCD		0x010
/* Accessed */
#define PTE_A		0x020
/* Dirty */
#define PTE_D		0x040
/* Page Size */
#define PTE_PS		0x080
/* Bits must be zero */
#define PTE_MBZ		0x180

static inline uint32_t va2kaddr(uint32_t addr) {
	return addr - MMAP_SEG_KERNEL_BASE;
}

static inline uint32_t uaddr2va(uint32_t addr) {
	return addr + MMAP_SEG_USER_BASE;
}

static inline uint32_t kaddr2va(uint32_t addr) {
	return addr + MMAP_SEG_KERNEL_BASE;
}

static inline uint32_t va2uaddr(uint32_t addr) {
	return addr - MMAP_SEG_USER_BASE;
}

static inline uint32_t uaddr2kaddr(uint32_t addr) {
	return va2kaddr(uaddr2va(addr));
}

static inline uint32_t kaddr2uaddr(uint32_t addr) {
	return va2uaddr(kaddr2va(addr));
}

struct PTE_t {
	uint32_t present : 1;
	uint32_t rw : 1;
	uint32_t user : 1;
	uint32_t __ : 9;
	uint32_t phys_pgno : 20; /* 12 bit >> physical page address */

	inline PTE_t(uint32_t physAddr, uint32_t perm) {
		*(uint32_t*)(this) = (physAddr & 0xFFFFF000) | (perm & 0xFFF);
	}

	inline uint32_t physAddr() {
		return (phys_pgno * 0x1000);
	}
	inline uint32_t perm() {
		return (*(uint32_t*)(this) & 0xFFF);
	}
	inline Page* phys_page() {
		return &pages[pageIndex(physAddr())];
	}
};

struct PageTable {
	PTE_t e[1024];
};

struct PDE_t {
	uint32_t present : 1;
	uint32_t rw : 1;
	uint32_t user : 1;
	uint32_t __ : 9;
	uint32_t phys_pgno : 20; /* 12 bit >> physical page address */

	inline PDE_t(uint32_t physAddr, int perm) {
		*(uint32_t*)(this) = (physAddr & 0xFFFFF000) | (perm & 0xFFF);
	}

	inline uint32_t physAddr() {
		return (phys_pgno * 0x1000);
	}
};

struct PageDirectory {
	PDE_t e[1024];
};

struct VA_t {
	uint32_t __ : 12;
	uint32_t ptx : 10;
	uint32_t pdx : 10;
	inline VA_t(uint32_t addr) {
		*this = *((VA_t*) &addr);
	}
	inline VA_t(uint32_t pdx, uint32_t ptx) {
		*(uint32_t*)this = (pdx<<22) + (ptx<<12);
	}
	inline void* addr() {
		return (void*) ((pdx * 1024 + ptx) * 0x1000);
	}
};

// FIXME: bunun burada olmamasi gerek
struct FuncLevelTester {
	uint32_t *level;
	inline FuncLevelTester(uint32_t *level) {
		this->level = level;
		(*level)++;
		ASSERT((*level)<3);
	}
	inline ~FuncLevelTester() {
		(*level)--;
	}
};


struct PageDirInfo {
	uint32_t pgdir_pa;
/* 1024 */
	PageDirectory *pgdir;
/* 1024 * 1024 */
	PageTable** pgtables;

	/* debug only */
	/* uint32_t function_level; */
    /* */

	uint32_t count_user;
	uint32_t count_kernel;
	uint32_t count_stack;
	uint32_t count_program;
	uint32_t count_shared;

	/* heap */
	uint32_t start_brk;
	uint32_t end_brk;

	inline PTE_t * page_get(VA_t va);
	inline PTE_t * page_get_c(VA_t va);
	inline int page_insert(struct Page *p, VA_t va, int perm);
	inline int page_remove(VA_t va);
	inline int pde_alloc(uint32_t pde_no);
	inline int pde_free(uint32_t pde_no);

	inline int page_alloc_insert(uint32_t va, int perm);
	int segment_alloc(uint32_t va, size_t len, int perm);
	int copy_pages(PageDirInfo *src, uint32_t start, uint32_t end);
	int link_pgtables(PageDirInfo *src, uint32_t start, uint32_t end);
	int link_pages(PageDirInfo *src, uint32_t start, uint32_t end, int perm);
	int verify_user_addr(const void *addr, size_t len, int perm);
};

extern int tmp_page_alloc_map(Page **p, uint32_t *va, int perm);
extern int tmp_page_free(uint32_t va);

inline PTE_t * PageDirInfo::page_get(VA_t va) {
	if (!pgtables[va.pdx])
		return NULL;
	return &pgtables[va.pdx]->e[va.ptx];
}

/** hata durumunda NULL dondurur */
inline PTE_t * PageDirInfo::page_get_c(VA_t va) {
	/* FuncLevelTester(&(this->function_level)); */
	if (!pgtables[va.pdx]) {
		/* create */
		if (pde_alloc(va.pdx) < 0)
			return NULL;
	}
	//TODO: ASSERT_D
	/* ASSERT(pgtables[va.pdx]); */
	return &(pgtables[va.pdx]->e[va.ptx]);
}

/** hatada errno, normal durumda refCount dondurur */
inline int PageDirInfo::page_insert(Page *p, VA_t va, int perm) {
	ASSERT(!(eflags_read() & FL_IF));

	/* FuncLevelTester(&(this->function_level)); */
	int r;
	PTE_t * pte = page_get_c(va);
	if (pte == NULL)
		return -ENOMEM;

	if (pte->present) {
		r = page_remove(va);
		ASSERT(r > -1);
	}

	int rc = p->refcount_inc();
	*pte = PTE_t(p->addr(), perm);

	uint32_t v = (uint32_t)va.addr();
	if (v >= MMAP_USER_BASE && v < MMAP_USER_TOP)
		count_user++;
	else if (v >= MMAP_KERNEL_BASE && v < MMAP_KERNEL_TOP)
		count_kernel++;
    else
		PANIC("page_insert: bilinmeyen adres alani");

	return rc;
}

inline int PageDirInfo::pde_alloc(uint32_t pde_no) {
	ASSERT(!(eflags_read() & FL_IF));

	/* pde olarak kullanmak icin, bir fiziksel page bul ve sıfırla */
	Page *p;
	uint32_t pt_va;
	if (tmp_page_alloc_map(&p, &pt_va, PTE_P | PTE_W) < 0)
		return -ENOMEM;
	pt_va = va2kaddr(pt_va);
	memset((void*)pt_va, 0, 0x1000);
	/* */
	pgdir->e[pde_no] = PDE_t(p->addr(),  PTE_P | PTE_U | PTE_W);
	pgtables[pde_no] = (PageTable*)pt_va;
	count_kernel++;
	return 0;
}

inline int PageDirInfo::pde_free(uint32_t pde_no) {
	ASSERT(!(eflags_read() & FL_IF));

	uint32_t pgtable_va = (uint32_t)pgtables[pde_no];
	int r = tmp_page_free(kaddr2va(pgtable_va));
	ASSERT(r == 0);
	count_kernel--;
	/*
	 * pagedir uzerinde not present olarak isaretle. Aslinda buna gerek yok
	 * ama simdilik hata kontrolu icin kullanisli olur.
	 */
	pgdir->e[pde_no].present = 0;
	pgtables[pde_no] = NULL;
	return 0;
}

/** hata durumunda errno, normal refCount dondurur */
inline int PageDirInfo::page_remove(VA_t va) {
	ASSERT(!(eflags_read() & FL_IF));

	/* FuncLevelTester(&(this->function_level)); */

	PTE_t *pte = page_get(va);
	if (pte == NULL || !pte->present)
		return -ENOMEM;

	int refCount = page_dec_refCount(pte->physAddr());
	*pte = PTE_t(0, 0);

	uint32_t v = (uint32_t)va.addr();
	if (v >= MMAP_USER_BASE && v < MMAP_USER_TOP)
		count_user--;
	else if (v >= MMAP_KERNEL_BASE && v < MMAP_KERNEL_TOP)
		count_kernel--;
    else
		PANIC("page_insert: bilinmeyen adres alani");

	// FIXME: --
	// tlb_invalidate();
	cr3_reload();
	return refCount;
}

/** hatada errno, normal durumda refCount dondurur */
inline int PageDirInfo::page_alloc_insert(uint32_t va, int perm) {
	ASSERT(!(eflags_read() & FL_IF));

	Page *p;
	int r;
	r = page_alloc(&p);
	if (r < 0)
		return r;

	r = page_insert(p, va, perm);
	if (r < 0)
		page_free(p);

	return r;
}

#endif //_MEMORY_VIRTUAL_H_
