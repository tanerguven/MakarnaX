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

#ifndef _MEMORY_KMALLOC_H_
#define _MEMORY_KMALLOC_H_

#include <types.h>
#include <wmc/list.h>
#include "virtual.h"
/*
 * algoritmayi daha iyi yapmak icin:
 * 32'lik bir alan alinmak isteniyorsa, 32lik alanlarin en cok oldugu pageden
 * alinmali.
 * Bu sekilde yapmak icin listeler, sayiya gore buyukten kucuge sirali
 * olmalidir.
 */

struct PH_count {
	uint32_t _32 : 8;
	uint32_t _64 : 8;
	uint32_t _128 : 8;
	uint32_t _256 : 8;
	uint32_t _512 : 4;
	uint32_t _1024 : 4;
	uint32_t _2048 : 4;
	uint32_t __ : 4; /* son bitleri byte tamamlamak icin */

	inline uint32_t get(uint32_t sizeno);
	inline void add(uint32_t sizeno, int i);

} __attribute__((packed));

#define FIRST_SIZENO 0
#define LAST_SIZENO 6

struct PageHeader {
	PH_count count;
	PH_count count_used;
};

enum BlockFlags {
	BlockFlag_free = 0x0055aaff & 0xfffc3fff,
	BlockFlag_used = 0xffaa5500 & 0xfffc3fff,
};

struct BlockHeader {
	uint32_t flags_1 : 14;
	uint32_t sizeno : 4; // size = 2^(5+sizeno)
	uint32_t flags_2 : 14;

	inline uint32_t page_addr() {
		return kaddr2va((uint32_t)this) & ~0xFFF;
	}

	inline PageHeader *ph() {
		extern PageHeader page_headers[];
		uint32_t off = kaddr2va((uint32_t)this) - MMAP_KERNEL_TMP_PAGE_BASE;
		uint32_t index = off / 0x1000;
		ASSERT(sizeno < LAST_SIZENO+2);
		ASSERT(index < 1024);
		return &page_headers[index];
	}

	inline uint32_t flags() { return *(uint32_t*)this & 0xFFFC3FFF; }
	inline void __set_free() { *(uint32_t*)this = (sizeno<<14) | BlockFlag_free; }
	inline void __set_used() { *(uint32_t*)this = (sizeno<<14) | BlockFlag_used; }

	inline uint32_t get_no() {
		uint32_t t = ((uint32_t)this) % 0x1000;
		return t / (1<<(sizeno+5));
	}

	inline bool is_free() { return flags() == BlockFlag_free; }

	static inline BlockHeader* bh(void *v)
		{ return (BlockHeader*)v - 1; }

	inline struct FreeBlock* fb()
		{ return (struct FreeBlock*)this; }

} __attribute__((packed));


struct LI_FreeBlock {
	static ptr_t offset_node_value;
};
typedef List_2<struct FreeBlock, LI_FreeBlock> FreeBlockList_t;


struct FreeBlock {
	BlockHeader header;
	FreeBlockList_t::node_t list_node;

	inline uint32_t size()
		{ return (1<<(5+header.sizeno)); }

	inline FreeBlock *right()
		{ return (FreeBlock*)(((uint32_t)this)+size()); }

	inline FreeBlock *left()
		{ return (FreeBlock*)(((uint32_t)this)-size()); }

	inline FreeBlock *pair() {
		uint32_t addr = (uint32_t)this;
		uint32_t _size = size();
		if (_size == 0x1000)
			return NULL;
		if ((addr / _size) % 2 == 0)
			return (FreeBlock*)(addr + _size);
		else
			return (FreeBlock*)(addr - _size);
	}

	inline void set_used() {
		ASSERT(list_node.is_free());
		header.__set_used();
		header.ph()->count_used.add(header.sizeno, 1);
	}

	inline void set_free() {
		ASSERT(!header.is_free());
		header.__set_free();
		header.ph()->count_used.add(header.sizeno, -1);
	}

	inline void* addr() {
		return (void*) (((uint32_t)this) + sizeof(BlockHeader));
	}

	void init();
	struct FreeBlock* split();
	struct FreeBlock* merge();
};

struct BlockSizeList {
	uint32_t size;
	FreeBlockList_t list;
};

inline uint32_t PH_count::get(uint32_t sizeno) {
	if (sizeno > 6)
		return -1;
	switch (sizeno) {
	case 6:
		return _2048;
	case 5:
		return _1024;
	case 4:
		return _512;
	default:
		uint8_t *a = (uint8_t*)this;
		return a[sizeno];
	};
	return -1;
}

inline void PH_count::add(uint32_t sizeno, int i) {
	if (sizeno > 6)
		return;

	switch (sizeno) {
	case 6:
		_2048 += i;
		break;
	case 5:
		_1024 += i;
		break;
	case 4:
		_512 += i;
		break;
	default:
		uint8_t *a = (uint8_t*)this;
		a[sizeno] += i;
	};
}

#endif /* _MEMORY_KMALLOC_H_ */
