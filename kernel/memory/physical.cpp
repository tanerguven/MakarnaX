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

#include <kernel/kernel.h>

#include "physical.h"
#include "../multiboot.h"
#include <errno.h>

/** Sanal bellegi etkinlestirmeden kullanilabilecek fonksiyonlar */
namespace PreVirtMem {
	uint32_t memoryEnd;
} // namespace PreVirtMem

struct Page* pages;
uint32_t pages_nr;
FreePageList freePageList;
uint32_t initrd_start = 0, initrd_end = 0;

/*
 * grub multiboot bilgilerini kullanarak belleğin boyutunu tespit eder
 * fiziksel page'lerin bilgilerinin saklanacağı dizi (pages) veri yapısını oluşturur
 * boş belleğin başlangıç noktasını, PrePaging'deki değişkene atar
 */
void detect_memory() {
	if (_multiboot_info->mods_count == 1) {
		multiboot_module_t *a = (multiboot_module_t*)_multiboot_info->mods_addr;
		initrd_start = a[0].mod_start;
		initrd_end = a[0].mod_end;
		print_info(">> initrd: %08x - %08x\n", initrd_start, initrd_end);
	} else if (_multiboot_info->mods_count > 1) {
		PANIC("birden fazla initrd desteklenmiyor");
	} else {
		print_info(">> initrd not found\n");
	}

	/* pages'i kernelin bittiği noktadan başlat */
	if (initrd_end > (uint32_t)&__kernel_end) {
		pages = (Page*)roundUp(initrd_end);
	} else {
		pages = (Page*)roundUp(&__kernel_end);
	}

	/* multiboot bilgisinden ram boyutunu okuyoruz ve page sayısını hesaplıyoruz */
    /* (1<<20) = 1MB = lowmem */
	uint32_t physicalEnd = (_multiboot_info->mem_upper<<10) + (1<<20);
	pages_nr = physicalEnd / 0x1000;
	/* pages'in bitiş noktası, kullanılan belleğin de bitiş noktası */
	PreVirtMem::memoryEnd = (uint32_t)roundUp(pages + pages_nr);
	/* bütün fiziksel page'leri bilinmeyen olarak işaretliyoruz */
	for (uint32_t i = 0 ; i < pages_nr ; i++)
		pages[i] = Page(Page::Type_unknown, 1);
}

// FIXME: ---
typedef multiboot_memory_map_t mb_mem_map_t;

/*
 * grub multiboot bilgilerine bakarak, boş bellek listesini oluşturuyor
 */
extern void pm_init() {
	freePageList.init();

	if (!(_multiboot_info->flags & MULTIBOOT_INFO_MEM_MAP)) {
		print_error("0x%08x\n", _multiboot_info);
		PANIC("multiboot memory map okunamiyor");
	}

	/* multiboot memory map'i tarıyoruz, uygun page'leri uygun olarak işaretle */
	for (	mb_mem_map_t *mmap = (mb_mem_map_t *)_multiboot_info->mmap_addr;
			(uint32_t)mmap < _multiboot_info->mmap_addr + _multiboot_info->mmap_length;
			mmap = (mb_mem_map_t *)((uint32_t) mmap + mmap->size + sizeof(mmap->size))
		)
	{
		if (mmap->type == MULTIBOOT_MEMORY_AVAILABLE) {
			uint32_t start = roundUp(mmap->addr_low);
			uint32_t end = roundDown(mmap->addr_low + mmap->len_low);

			/* 4 GB'dan buyuk fiziksel adresleri kullanamaz */
			if (mmap->addr_high > 0)
				continue;

			for (uint32_t i = start ; i < end; i += 0x1000) {
				if (i < 0x100000) {
					/*
					 * low memory'deki alanı uygun fakat serbest değil olarak
					 * işaretliyoruz, bu alan real mod için gerekli
					 */
					pages[pageIndex(i)] = Page(Page::Type_available, 1);
					pages[pageIndex(i)].nextFreeIndexOrRefCount = 1;
				} else if (i < PreVirtMem::memoryEnd) {
					pages[pageIndex(i)] = Page(Page::Type_kernel, 1);
				} else {
					/* free page listesine ekliyoruz */
					Page *p = &pages[pageIndex(i)];
					*p = Page(Page::Type_available, 0);
					page_free(p);
				}
			}
		}
	}
}

uint32_t mem_total() {
    return pages_nr << 12;
}

uint32_t mem_lowFree() {
	return roundUp(_multiboot_info->mem_lower<<10);
}
