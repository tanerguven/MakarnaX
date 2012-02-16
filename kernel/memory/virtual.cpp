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

#include "../kernel.h"
#include <string.h>

#include <types.h>
#include <asm/x86.h>
#include <data_structures/bitmap.h>

#include "virtual.h"
#include "physical.h"
#include "../trap.h"

#include <errno.h>

// memory_phys.cpp
extern int page_dec_refCount(uint32_t paddr);
extern int page_alloc(Page **p);
extern void page_free(Page *p);
namespace PreVirtMem {
	extern uint32_t memoryEnd;
} // namespace PreVirtMem
//

//
static void tmp_page_init(uint32_t pa);
int tmp_page_alloc_map(Page **p, uint32_t *va, int perm);
//

SegmentDesc gdt[10];
PseudoDesc gdt_pd;

PageDirInfo kernel_dir;

void* __init_free_pages[10];

/** aciklama icin paging.h ve memory.h */
void vm_init() {
	Page *p;
	int err;
	err = page_alloc(&p);
	ASSERT(err == 0);
	kernel_dir.pgdir = (PageDirectory*)p->addr();
	kernel_dir.pgdir_pa = (uint32_t)kernel_dir.pgdir;
	memset(kernel_dir.pgdir, 0, 0x1000);

	err = page_alloc(&p);
	ASSERT(err == 0);
	kernel_dir.pgtables = (PageTable**)p->addr();
	memset(kernel_dir.pgtables, 0, 0x1000);

	for (int i = 0 ; i < 10 ; i++) {
		page_alloc(&p);
		__init_free_pages[i] = (void*)p->addr();
		memset(__init_free_pages[i], 0, 0x1000);
	}

	/*
	 * paging için kullanacağımız veri yapısı(page table), page kullanımının
	 * artmasına sebep oluyor.
	 * örnek hesaplar:
	 * ---------------------------
	 * pageCount: 1022360
	 * pageTableCount: 999
	 * ->
	 * pageCount: 1023384
	 * pageTableCount: 1000
	 * -> page table sayısı 1 artıyor
	 * ----------------------------
	 * pageCount: 1024
	 * pageTableCount: 1
	 * ->
	 * pageCount: 1026
	 * pageTableCount: 2
	 * -> ++
	 */
	uint32_t pageCount = PreVirtMem::memoryEnd / 0x1000 + 1;
	uint32_t pageTableCount = pageCount / 1024 + 1;
	if (pageTableCount * 1024 < pageCount + pageTableCount)
		pageTableCount++;
	pageCount+=pageTableCount;
	/* */

	/* 1 - kullanılan fiziksel belleği, hem +0xc0000000 hem de 1-1 map ediyoruz */
	uint32_t mappedPageCount = 0;
	for (uint32_t i = 0; i < pageTableCount ; i++) {
		Page *p;
		page_alloc(&p);
		uint32_t pt = p->addr();

#if MMAP_SEG_KERNEL_BASE != 0
		/* kernel segmentation kullanilarak calistirilacaksa */
		kernel_dir.pgdir->e[i+VA_t(MMAP_KERNEL_BASE).pdx] = PDE_t(pt, PTE_P | PTE_W);
		kernel_dir.pgtables[i+VA_t(MMAP_KERNEL_BASE).pdx] = (PageTable*)pt;
#endif
		kernel_dir.pgdir->e[i] = PDE_t(pt, PTE_P | PTE_W);
		kernel_dir.pgtables[i] = (PageTable*)pt;

		for (uint32_t j = 0 ; j < 1024; j++) {
			if (mappedPageCount > pageCount) {
				kernel_dir.pgtables[i]->e[j] = PTE_t(0, 0);
			} else {
				uint32_t pa = (i * 1024 + j) * 0x1000;
				if ((pa >= (uint32_t)&__text_start) && (pa < (uint32_t)&__rodata_end)) {
					/* text ve read only data bölümlerine yazma izni yok */
					kernel_dir.pgtables[i]->e[j] = PTE_t(pa, PTE_P);
				} else {
					kernel_dir.pgtables[i]->e[j] = PTE_t(pa, PTE_P | PTE_W);
				}
				mappedPageCount++;
			}
		}
	}
	load_reg(%cr3, kernel_dir.pgdir_pa);

	/* enable paging */
	uint32_t cr0; read_reg(%cr0, cr0);
	cr0 |= CR0_PE|CR0_PG|CR0_AM|CR0_NE|CR0_TS|CR0_EM|CR0_MP|CR0_WP;
	cr0 &= ~(CR0_TS|CR0_EM);
	load_reg(%cr0, cr0);
	printf(">> cr0 OK\n");
	/* */

	/* 2 - segmentation'u etkinleştir */
	for (uint32_t i = 0 ; i < sizeof(gdt) / sizeof(gdt[0]) ; i++)
		gdt[i].set_segNull();
	gdt_pd.base = kaddr2va((uint32_t)gdt);
	gdt_pd.lim = sizeof(gdt) - 1;
	gdt[GD_KT >> 3].set_seg(STA_X | STA_R, MMAP_SEG_KERNEL_BASE, 0xffffffff, 0);
	gdt[GD_KD >> 3].set_seg(STA_W, MMAP_SEG_KERNEL_BASE, 0xffffffff, 0);
	gdt[GD_UT >> 3].set_seg(STA_X | STA_R, MMAP_SEG_USER_BASE, 0xffffffff, 3);
	gdt[GD_UD >> 3].set_seg(STA_W, MMAP_SEG_USER_BASE, 0xffffffff, 3);
	gdt[GD_TSS >> 3].set_segNull();
	gdt_load((uint32_t)&gdt_pd);
	load_reg(%gs, GD_UD | 3);
	load_reg(%fs, GD_UD | 3);
	load_reg(%es, GD_KD);
	load_reg(%ds, GD_KD);
	load_reg(%ss, GD_KD);
	cs_set(GD_KT);
	asm volatile("lldt %%ax" :: "a" (0));
	printf(">> segmentation OK\n");
	/* */

#if MMAP_SEG_KERNEL_BASE != 0
/* 3 - Geçici olarak fiziksel bellek ile 1-1 map ettiğimiz alanı iptal et */
	for (uint32_t i = 0 ; i < (mappedPageCount >> 10) + 1 ; i++) {
		kernel_dir.pgdir->e[i] = PDE_t(0 ,0);
		kernel_dir.pgtables[i] = 0;
	}
	cr3_reload();
#endif

	tmp_page_init((uint32_t)__init_free_pages[0]);
}

/** hatada errno, normalde 0 dondurur */
int PageDirInfo::segment_alloc(uint32_t va, size_t len, int perm) {
	ASSERT(!(eflags_read() & FL_IF));

	/* debug only */
	uint32_t mem_before_segment_alloc = mem_free();
	uint32_t count_kernel_before_segment_alloc = count_kernel;
	/* */
	Page *p;
	int err, r;
	uint32_t i;
	uint32_t end = va + len;
	va = roundDown(va);
	end = roundUp(end);

	for (i = va ; i < end ; i += 0x1000) {
		err = page_alloc(&p);
		if (err < 0)
			goto bad_segment_alloc;

		err = page_insert(p, VA_t(i), perm);
		if (err < 0) {
			page_free(p);
			goto bad_segment_alloc;
		}
	}

	return 0;

bad_segment_alloc:
	end = i;
	/* free pages */
	for (i = va ; i < end ; i+= 0x1000) {
		r = page_remove(i, 1);
		ASSERT(r == 0);
	}

	/* free pages pgtables */
	uint32_t va_pde = VA_t(va).pdx;
	uint32_t end_pde = VA_t(end).pdx;
	for (i = va_pde ; i < end_pde ; i++) {
	    pde_free(i);
	}

	ASSERT(count_kernel == count_kernel_before_segment_alloc);
	ASSERT(mem_free() == mem_before_segment_alloc);
	return err;
}

/**
 * simdiki pgdir'dan dest_pgdir'a page kopyalar.
 * hatada errno, normalde 0 dondurur.
 */
static inline int copy_page_from_current(PageDirInfo *dest_pgdir, uint32_t va, int perm) {
	ASSERT(!(eflags_read() & FL_IF));

	/*
	 * FIXME: Sacma bir yontem olabilir. Gecici map ederek fiziksel pageleri
	 * kopyala. Sanal adresi kullanmadan fiziksel olarak kopyalamanin bir
	 * yontemi olabilir.
	 * Farkli olarak paging kapatilarak yapilabilir. Bence VM kapatmak daha
	 * sacma bir cozum. (Her page icin pagingi ve segmentationu kapa/ac)
	 *
	 * Simdilik cache-disable olarak gecici map ederek yapiyoruz.
	 */
	int r;
	Page *p;
	uint32_t tmp_va;

	r = tmp_page_alloc_map(&p, &tmp_va, PTE_P | PTE_W);

	if (r < 0)
		return r;

	memcpy((void*)va2kaddr(tmp_va), (void*)va2kaddr(va), 0x1000);

	r = dest_pgdir->page_insert(p, va, perm & 0x7);
	if (r < 0) {
		ASSERT(tmp_page_free(tmp_va) == 0);
		return r;
	}

	r = tmp_page_free(tmp_va);

	ASSERT(r > -1);

	return 0;
}

static inline int copy_page(PageDirInfo *dest, PageDirInfo *src, uint32_t va, int perm) {
	ASSERT(!(eflags_read() & FL_IF));

	PANIC("tamamlanmadi");
	return -1;
}

/** hatada errno, normalde 0 dondurur */
int PageDirInfo::copy_pages(PageDirInfo *src, uint32_t start, uint32_t end) {
	ASSERT(!(eflags_read() & FL_IF));
	uint32_t cr3; read_reg(%cr3,cr3);
	if (src->pgdir_pa == cr3) {
		for (uint32_t i = start ; i < end ; i+=0x1000) {
			PTE_t *pte = src->page_get(i);

			if (!pte | !pte->present)
				continue;

			int err = copy_page_from_current(this, i, pte->perm());
			if (err < 0)
				return err;

			// FIXME: tlb_invalidate hatali, gecici cozum (kvm'de hatali calisiyor)
			cr3_reload();
		}

		return 0;

	}
	PANIC("tamamlanmadi");
	return -1;
}


int PageDirInfo::link_pgtables(PageDirInfo *src, uint32_t start, uint32_t end) {
	for (uint32_t i = VA_t(start).pdx; i < VA_t(end).pdx ; i++) {
		this->pgdir->e[i] = src->pgdir->e[i];
		this->pgtables[i] = src->pgtables[i];
	}
	return 0;
}

int PageDirInfo::link_pages(PageDirInfo *src, uint32_t start, uint32_t end, int perm) {
	ASSERT(!(eflags_read() & FL_IF));
	int r;

	for (uint32_t i = start; i < end ; i+=0x1000) {
		PTE_t *src_pte = src->page_get(VA_t(i));
		if (!src_pte || !src_pte->present )
			continue;

		r = this->page_insert(src_pte->phys_page(), VA_t(i), perm);
		if (r < 0)
			goto bad_link_pages;
	}
	return 0;

bad_link_pages:
	uint32_t end_pde = VA_t(end).pdx;
	for (uint32_t i = VA_t(start).pdx ; i < end_pde ; i++)
	    pde_free(i);
	return -ENOMEM;
}


int PageDirInfo::verify_user_addr(const void *addr, size_t len, int perm) {
	uint32_t va = uaddr2va((uint32_t)addr);

	if ( va >= MMAP_USER_TOP || va + len > MMAP_USER_TOP)
		return -1;

	if ( va < MMAP_USER_BASE || va + len < MMAP_USER_BASE)
		return -1;

	for (uint32_t i = va ; i < roundUp(va+len) ; i+=0x1000) {
		PTE_t *pte = page_get(i);
		if (pte == NULL)
			return -1;
		if ( !pte->present || (perm & !(pte->perm() & perm)))
			return -1;
	}

	return 0;
}


/**********************************************************
 * tmp_page_alloc
 ***********************************************************/

// FIXME: burada kullanilabilecek butun adreslerin pgtable lari olusturulmali
#define TMP_PAGE_COUNT 1024
uint32_t tmp_pages[TMP_PAGE_COUNT/32];

static void tmp_page_init(uint32_t pa) {
	memset(tmp_pages, 0, sizeof(tmp_pages));
	/* tmp_page için, page table */
    kernel_dir.pgdir->e[VA_t(MMAP_KERNEL_TMP_PAGE_BASE).pdx] = PDE_t(pa, PTE_P | PTE_W);
	kernel_dir.pgtables[VA_t(MMAP_KERNEL_TMP_PAGE_BASE).pdx] = (PageTable*)pa;
	/* */
}

static inline uint32_t tmp_page_get() {
	ASSERT(!(eflags_read() & FL_IF));

	int r = bitmap_find_first_zero(tmp_pages, sizeof(tmp_pages));
	if (r > -1) {
		bitmap_set(tmp_pages, r);
		return MMAP_KERNEL_TMP_PAGE_BASE + ( r * 0x1000 );
	}
	ASSERT(r != -2);
	return 0;
}

static inline void tmp_page_put(uint32_t va) {
	ASSERT(!(eflags_read() & FL_IF));

	uint32_t i = (va - MMAP_KERNEL_TMP_PAGE_BASE) / 0x1000;
	ASSERT( bitmap_test(tmp_pages, i) );
	bitmap_reset(tmp_pages, i);
}

/**
 * boş fiziksel page ve virtual adres bulur, map eder
 * kernelde malloc yerine kullanilabilir
 */
int tmp_page_alloc_map(Page **p, uint32_t *va, int perm) {
	ASSERT(!(eflags_read() & FL_IF));

	int err;
	*va = tmp_page_get();
	if (*va == 0) {
		// TODO: buna bir cozum bulunmali
		// printf(">> tmp page bitti\n");
		return -ENOMEM;
	}

	err = page_alloc(p);
	if (err < 0)
		goto bad_tmp_page_alloc_map__page_alloc;

	err = kernel_dir.page_insert(*p, VA_t(*va), perm);
	if (err < 0)
		goto bad_tmp_page_alloc_map__page_insert;

	return 0;

bad_tmp_page_alloc_map__page_insert:
	page_free(*p);
bad_tmp_page_alloc_map__page_alloc:
	tmp_page_put(*va);
	return err;
}

int tmp_page_free(uint32_t va) {
	ASSERT(!(eflags_read() & FL_IF));

	// FIXME: MAMP_KERNEL_TMP_PAGE_TOP
	ASSERT(va >= MMAP_KERNEL_TMP_PAGE_BASE && va < MMAP_KERNEL_TOP);

	int r;
	r = kernel_dir.page_remove(va, 1);
	if (r > -1)
		tmp_page_put(va);
	return r;
}
