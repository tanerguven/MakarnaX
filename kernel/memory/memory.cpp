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

#include <string.h>
#include <stdio.h>
using namespace std;
#include "../panic.h"

#include <types.h>
#include <asm/x86.h>

#include "physical.h"
#include "virtual.h"

// memory_physical.cpp
extern void detect_memory();
extern void pm_init();
//

// memory_virtual.cpp
extern void vm_init();
//

// kmalloc.cpp
extern void test_kmalloc();
extern void kmalloc_init();
//

int memory_init() {
    /*
	 * multiboot bilgilerinden bellek boyutunu tespit et ve fiziksel page
	 * dizisini oluştur
	 */
	detect_memory();

	/* fiziksel page dizisindeki boş alanları işaretle ve liste oluştur */
	pm_init();

	/* kerneli 0xc0000000 adresinde map et ve gdt base'i 0xc0000000 yap */
	vm_init();

	kmalloc_init();

	/* page test */
	// test_free_pages();
	// printf(">> test_free_pages OK\n");
	/* */

	/* kmalloc test */
	test_kmalloc();
	printf(">> test_kmalloc OK\n");

	return 0;
}


/**********************************************************
 * test
 **********************************************************/
static void test_free_pages() {
	extern FreePageList freePageList;
	extern PageDirInfo kernel_dir;
	extern int page_alloc(Page **p);

	Page *p;
	/* test FreePageList structure and functions */
	FreePageList testList;
	testList.init();
	ASSERT(testList._begin == NULL);
	ASSERT(testList._end == NULL);
	ASSERT(testList._count == 0);

	uint32_t free_page_count = freePageList._count;
	while (freePageList._count) {
		page_alloc(&p);
		ASSERT(p);
		ASSERT(p->type == Page::Type_available);
		ASSERT(p->refcount_get() == 0);
		p->free = true;
		testList.insert(p);
		ASSERT(p->type == Page::Type_available);
		ASSERT(p->refcount_get() == 0);
		ASSERT(p->free);
	}

	ASSERT(free_page_count == testList._count);
	ASSERT(page_alloc(&p) < 0);

	while (testList._count) {
		Page* p = testList.getFirst();
		ASSERT(p);
		ASSERT(p->type == Page::Type_available);
		ASSERT(p->refcount_get() == 0);
		ASSERT(p->free);
		p->nextFreeIndexOrRefCount = 1;
		p->free = false;
		p->refcount_dec();
	}
	ASSERT(free_page_count == freePageList._count);

	printf(">> Free Page List Test OK\n");
	/* */

	/* test physical pages */
	/*
	 * free page listesindeki elemanların kullanılabilirliğini test etmek için
	 * döngüyle 0xE0000000 0xF0000000 aralığına bütün pageleri bağla ve
	 * 0 lar ve 1 ler yaz.
	 */
	if (mem_total() > 250 * 1024) {
		// FIXME: silinen page'ler icin bir liste olusturulmali
		PANIC("test physical pages 250mb üstü bellekle calismak icin ayarlanmadi");
	}
	while (1) {
		for (uint32_t i = 0xE0000000; i < 0xF0000000 ; i += 0x1000) {
			if (page_alloc(&p) < 0)
				goto end_while;
			ASSERT(!p->free);
			kernel_dir.page_insert(p, VA_t(i), PTE_P);
			memset((void*)va2kaddr(i), 0, 0x1000);
			memset((void*)va2kaddr(i), 0xFF, 0x1000);
		}
	} end_while:
	ASSERT(freePageList._count == 0);
	printf(">> Physical Pages Test OK\n");

    for (uint32_t i = 0xE0000000 ; i < 0xF0000000 ; i += 0x1000) {
		kernel_dir.page_remove(VA_t(i));
	}
	/* */
}
