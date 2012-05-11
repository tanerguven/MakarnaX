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
 * Bu dosya, fiziksel bellek yonetimi ile ilgili veri yapilarini icerir.
 */

#ifndef _MEMORY_PHYSICAL_H_
#define _MEMORY_PHYSICAL_H_
#include <asm/x86.h>

#include <kernel/kernel.h>
#include <genel_fonksiyonlar.h>
#include <errno.h>

/* link.ld */
extern void* __text_start;
extern void* __text_end;
extern void* __rodata_start;
extern void* __rodata_end;
extern void* __data_start;
extern void* __data_end;
extern void* __bss_start;
extern void* __bss_end;
extern void* __kernel_end;

/* memory_phys.cpp */
extern uint32_t mem_total();
extern uint32_t mem_lowFree();

/*
 * fiziksel page'lerin bilgileri, pages dizisi ve freePageList listesi ile
 * saklanıyor.
 *
 * not:
 * 	veriyapısından dolayı page 0 malloc, free fonksiyonlarıyla kullanılamaz,
 *  Null olarak kullanılıyor, zaten page 0 dolu, içinde kesme vektörü var.
 */
/** fiziksel page'lerin bilgilerinin saklandığı dizi */
extern struct Page* pages;
/** fiziksel page'lerin sayısı */
extern uint32_t pages_nr;
/** fiziksel adresin pages dizisindeki indexini hesaplar */
static inline uint32_t pageIndex(uint32_t pa) {
	return pa/0x1000;
}

/**
 * fiziksel page bilgilerinin saklandığı yapı.
 * boş listesi için, önceki ve sonraki boş'un index'i saklanıyor
 */
struct Page {
	uint32_t nextFreeIndexOrRefCount : 20;
	// bool free; -> bu şekilde kullaninca 1 bit olmayip hata verebiliyor
	uint32_t free : 1; // yada bool free : 1; olmalı
	uint32_t type : 3;
	uint32_t __ : 8;

	enum Type {
		Type_unknown = 0,
		Type_available = 1,
		Type_kernel = 2,
	};

	inline Page(Type type, uint32_t refCount)
		: nextFreeIndexOrRefCount(refCount), free(0), type(type) {}

	inline Page* nextFree()
		{ return (free) ? (pages + nextFreeIndexOrRefCount) : 0; }
	inline uint32_t index()
		{ return ((uint32_t)this-(uint32_t)pages) / sizeof(Page); }
	inline uint32_t addr()
		{ return index() * 0x1000; }

	inline int refcount_get()
		{ return (free) ? 0 : nextFreeIndexOrRefCount; }

	inline int refcount_dec();

	inline int refcount_inc()
		{ ASSERT(!free); return ++nextFreeIndexOrRefCount; }
};

/**
 * pages dizisindeki boş page'lerin listesi
 * listedeki her eleman önceki ve sonraki boş elemanın dizi indexini saklıyor
 * (pages + dizi indexi) hesaplamasıyla elemanlara erişiliyor.
 * index 0, NULL u temsil ediyor.
 */
struct FreePageList {
	struct Page* _begin;
	struct Page* _end;
	uint32_t _count;
	inline void init() {
		_begin = _end = NULL;
		_count = 0;
	}
	inline void insert(struct Page* node) {
		ASSERT(node->index() != 0);
		node->nextFreeIndexOrRefCount = 0;
		if (_begin) {
			_end->nextFreeIndexOrRefCount = node->index();
			_end = node;
		} else {
			_begin = _end = node;
		}
		_count++;
	}
	inline Page* getFirst() {
		if (! _begin)
			return NULL;
		_count--;
		Page *p = _begin;
		_begin = _begin->nextFree();
		p->nextFreeIndexOrRefCount = 0;
		if (_end == p)
			_end = _begin = NULL;
		return p;
	}
};

extern FreePageList freePageList;

inline int page_dec_refCount(uint32_t paddr) {
	ASSERT_int_disable();
	ASSERT(isRounded(paddr));
	Page *p = &pages[pageIndex(paddr)];
	return p->refcount_dec();
}

/**
 * boş frame listesindeki elemanlardan ilkini listeden çıkarır ve adresini
 * döndürür.
 */
inline int page_alloc(Page **p) {
	int r;
	ASSERT_int_disable();

	*p = freePageList.getFirst();
	if (*p) {
		ASSERT((*p)->free);
		ASSERT((*p)->type = Page::Type_available);
		(*p)->free = false;
		r = 0;
	} else {
		r = -ENOMEM;
	}

	return r;
}

/* girilen frame adresini boş listesine ekler */
static inline void page_free(Page *p) {
	ASSERT_int_disable();

	ASSERT(p->type == Page::Type_available);
	ASSERT(! p->free);
	ASSERT(p->refcount_get() == 0);
	if (p->refcount_get() == 0) {
		p->free = true;
		freePageList.insert(p);
	}
}

inline int Page::refcount_dec() {
	ASSERT_int_disable();

	ASSERT(!free);
	ASSERT(this->index() != 0);
	nextFreeIndexOrRefCount--;
	int rc = this->refcount_get();
	ASSERT(rc > -1);
	if (rc == 0)
		page_free(this);

	return rc;
}

inline uint32_t mem_free() {
	return freePageList._count << 12;
}

#endif //_MEMORY_PHYSICAL_H_
