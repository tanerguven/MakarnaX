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

#include <string.h>

#include "kmalloc.h"
#include "virtual.h"

static BlockSizeList sizes[] = {
	{ 32, FreeBlockList_t() },
	{ 64, FreeBlockList_t() },
	{ 128, FreeBlockList_t() },
	{ 256, FreeBlockList_t() },
	{ 512, FreeBlockList_t() },
	{ 1024, FreeBlockList_t() },
	{ 2048, FreeBlockList_t() },
};
#define n_sizes (sizeof(sizes)/sizeof(sizes[0]))

PageHeader page_headers[1024];

ptr_t LI_FreeBlock::offset_node_value = (ptr_t)offsetof(FreeBlock,list_node);

void kmalloc_init() {
	ASSERT(sizeof(FreeBlock) <= (1<<(FIRST_SIZENO+5)));

	for (uint32_t i = 0 ; i < n_sizes ; i++) {
		sizes[i].list.init();
	}
}

size_t kmalloc_size(size_t size) {
	uint32_t s = 32;
	do {
		s = s<<1;
	} while (size + sizeof(BlockHeader) > s);
	ASSERT(s < 0x1000);
	return s;
}

void *kmalloc(size_t size) {
	ASSERT_int_disable();
	int i;
	int uygun_sizeno = -1;
	FreeBlock *b = NULL;

	if (size > (2048 - sizeof(BlockHeader))) {
		print_warning(">> kmalloc icin cok buyuk\n");
		return NULL;
	}

	for (i = FIRST_SIZENO ; i < LAST_SIZENO+1 ; i++) {
		if (sizes[i].size - sizeof(BlockHeader) >= size) {
			if (uygun_sizeno == -1)
				uygun_sizeno = i;
			if (sizes[i].list.size() > 0) {
				b = sizes[i].list.front();
				ASSERT(b->header.sizeno == i);
				ASSERT( sizes[i].list.pop_front() );
				break;
			}
		}
	}

	ASSERT(uygun_sizeno != -1);

	if (i == LAST_SIZENO+1) {
		ASSERT(sizes[LAST_SIZENO].list.size() == 0);
		Page *p;
		uint32_t va;
		if (tmp_page_alloc_map(&p, &va, PTE_P | PTE_W) < 0)
			return NULL;
		b = (FreeBlock*)va2kaddr(va);
		// print_info("page alindi %08x\n", b->header.page_addr());
		b->init();
		FreeBlockList_t::iterator r = b->list_node.__list->erase(&b->list_node);
		ASSERT( r != FreeBlockList_t::error());
		i = LAST_SIZENO;
	}

	ASSERT(b);
	ASSERT(b->list_node.is_free());

#if 1
	// split ozelligi kapatilabilir
	for ( ; i > uygun_sizeno ; i--) {
		ASSERT(b->header.sizeno == i);
		b = b->split();
	}
#endif

	b->set_used();

	return b->addr();
}

void kfree(void *v) {
	ASSERT_int_disable();

	BlockHeader *bh = BlockHeader::bh(v);
	ASSERT(bh->flags() == BlockFlag_used);

	FreeBlock *fb = bh->fb();
	fb->set_free();

#if 1
	// split ozelligi kapatilabilir
	while ((fb->header.sizeno < LAST_SIZENO+1) && fb->pair()->header.is_free() &&
		   (fb->pair()->header.sizeno == fb->header.sizeno)) {
		fb = fb->merge();
	}
#endif

	ASSERT(fb->list_node.is_free());
	if (fb->header.sizeno <= LAST_SIZENO) {
		bool b = sizes[fb->header.sizeno].list.push_front(&fb->list_node);
		ASSERT(b);
	} else {
		uint32_t va = kaddr2va((uint32_t)fb->header.page_addr());
		ASSERT(fb->size() == 0x1000);
		// print_info(">> page %08x tamamen bos, siliniyor\n", va);
		tmp_page_free(va2kaddr(va));

		// FIXME: burada bir bug var. kvm'de cr3_reload yapmayinca sacma hatalar veriyor.
		cr3_reload();
	}
}


void FreeBlock::init() {
	header.sizeno = LAST_SIZENO+1;
	header.__set_free();

	// header.__ph = &page_headers[next_page_header++];
	memset(header.ph(), 0, sizeof(PageHeader));

	list_node.init();

	FreeBlock* b = split();
	ASSERT( sizes[b->header.sizeno].list.push_back(&b->list_node) );
}

FreeBlock* FreeBlock::split() {
	int used = header.ph()->count_used.get(header.sizeno-1);
	int count = header.ph()->count.get(header.sizeno-1);
	int r;

	/* bosta parcalanan boyutta olmamasi lazim */
	ASSERT(count == used);

	if (list_node.__list)
		list_node.__list->erase(&list_node);

	header.sizeno--;
	header.ph()->count.add(header.sizeno+1, -1);
	header.ph()->count.add(header.sizeno, 2);

	// sizes[header.sizeno].list.push_back(&list_node);

	ASSERT(pair() == right());

	FreeBlock *_right = right();
	_right->header = header;
	_right->list_node.init();

	r = sizes[header.sizeno].list.push_back(&_right->list_node);
	ASSERT(r);

	return this;
}

FreeBlock* FreeBlock::merge() {
	FreeBlock *_pair = pair();
	ASSERT(_pair->header.is_free());
	ASSERT(header.is_free());
	ASSERT(_pair->header.sizeno == header.sizeno);

	ASSERT(list_node.is_free());

	// pair free olduguna gore free listesinde olmak zorunda
	ASSERT( !_pair->list_node.is_free() );
	_pair->list_node.__list->erase(&_pair->list_node);

	header.ph()->count.add(header.sizeno, -2);
	header.ph()->count.add(header.sizeno+1, 1);

	FreeBlock *r = this;

	if (_pair == left()) {
		r = _pair;
	}

	r->header.sizeno++;
	ASSERT(r->list_node.is_free());

	return r;
}

/*******************************************************
 * test
 *******************************************************/

#include <string.h>

static void test1();
static void test2();
static void test3();
static void test4();
static void compare_total_count();

static void *x[100];
static BlockHeader *bh[100];
static FreeBlock *fb[100];
static uint32_t i = 0;

static uint32_t x_sizeno[30];
static uint32_t x_size[30];

static PH_count c[2];
static PH_count cu[2];

void test_kmalloc() {
	test1();
	// print_info("test 1 OK\n");

	/* test 2 aradan rastgele birseyler siliyoruz */
	test2();
	// print_info("test 2 OK\n");

	/* test 3 rastgele boyutta veriler ekle ve rasgele sil */
	test3();
	// print_info("test 3 OK\n");

	/* test 3'e devam */
	test4();
	// print_info("test 4 OK\n");

	return;
}

#if 0
static void listele(PageHeader *ph) {
	print_info("\n");
	print_info("size:\tcount\tused\n");
	for (int i = LAST_SIZENO ; i > FIRST_SIZENO-1 ; i--) {
		print_info("%d:\t%d\t%d\n", 1<<(5+i), ph->count.get(i),
			   ph->count_used.get(i));
	}
}
#endif

static void test1() {
	i = 0;
	for ( ; i < 5 ; ) {
		// print_info(">> malloc %d\n", i);
		x[++i] = kmalloc(1);
		bh[i] = BlockHeader::bh(x[i]);
		kfree(x[i]);
		ASSERT( x[i] == kmalloc(1) );
		ASSERT( bh[i]->get_no() == i-1);
		ASSERT( !bh[i]->is_free() );
		ASSERT( bh[i]->ph()->count_used.get(0) == i );
	}

	/* ortadan bir eleman sil */
	kfree(x[3]);
	ASSERT( bh[3]->is_free() );
	ASSERT( bh[3]->get_no() == 2 );
	ASSERT( bh[3]->ph()->count_used.get(0) == 4 );
	ASSERT( bh[3]->ph()->count.get(0) == 6 ); //ortadan sildik, birlestirmedi
	fb[3] = bh[3]->fb();
	ASSERT( !fb[3]->right()->header.is_free());
	ASSERT( !fb[3]->left()->header.is_free());
	ASSERT( x[3] == kmalloc(1) );

	/* bastan sil */
	kfree(x[1]);
	ASSERT( x[1] == kmalloc(1) );

	/* sondan sil */
	kfree(x[5]);
	ASSERT( x[5] == kmalloc(1) );

	/* hepsini sil */
	do {
		// print_info(">> free %d\n", i);
		kfree(x[i]);
	} while (--i > 0);

	/* tekrar al */
	i = 0;
	for ( ; i < 5 ; ) {
		// print_info(">> malloc %d\n", i);
		ASSERT( x[++i] = kmalloc(1) );
	}

	/* artan boyutlarda bellek al */
	for ( ; i < 10 ; ) {
		// print_info(">> malloc %d\n", i);
		++i;
		x[i] = kmalloc(i * 7);
		bh[i] = BlockHeader::bh(x[i]);
		kfree(x[i]);
		ASSERT( x[i] == kmalloc(i * 7) );
	}

	/* 2 tane haric hepsini sil */
	do {
		if ((i == 3) | (i == 9))
			i--;
		// print_info(">> free %d\n", i);
		kfree(x[i]);
	} while (--i > 0);

	for (i=0 ; i < 5 ; ) {
		if (i == 2)
			i++;
		// print_info(">> malloc %d\n", i);
		ASSERT( x[++i] = kmalloc(1) );
	}

	for ( ; i < 10 ; ) {
		if (i == 8)
			i++;
		// print_info(">> malloc %d\n", i);
		ASSERT( x[++i] = kmalloc(1) );
	}

	/* hepsini sil */
	i = 10;
	do {
		// print_info(">> free %d\n", i);
		kfree(x[i]);
	} while (--i > 0);
}

static void test2() {
	memset(x_sizeno, 0, sizeof(x_sizeno));
	memset(x_size, 0, sizeof(x_size));

	for (i = 0 ; i < 30 ; ) {
		++i;
		int b = i * 11;
		// print_info(">> malloc %d\n", i);
		x[i] = kmalloc(b);

		bh[i] = BlockHeader::bh(x[i]);
		x_size[i] = b;
		x_sizeno[i] = bh[i]->sizeno;

		kfree(x[i]);
		ASSERT( x[i] ==  kmalloc(b));
	}

	c[0] = page_headers[0].count;
	c[1] = page_headers[1].count;
	cu[0] = page_headers[0].count_used;
	cu[1] = page_headers[1].count_used;

	for (int j = 7 ; j < 23 ; j++) {
		if (j % 5 == 2 || j % 5 == 3) {
			// print_info("free %d\n", j);
			kfree(x[j]);
		}
	}

	for (int j = 7 ; j < 23 ; j++) {
		if (j % 5 == 2 || j % 5 == 3) {
			// print_info("malloc %d\n", j);
			uint32_t b = j * 11;
			x[j] = kmalloc(b);
			ASSERT( x_size[j] == b);
			ASSERT( x_sizeno[j] == BlockHeader::bh(x[j])->sizeno);
		}
	}

	ASSERT( memcmp(&c, &bh[7]->ph()->count, sizeof(PH_count)) == 0);
	ASSERT( memcmp(&cu, &bh[7]->ph()->count_used, sizeof(PH_count)) == 0);

	for (int j = 7 ; j < 23 ; j++) {
		if (j % 5 == 1 || j % 5 == 2) {
			// print_info("free %d\n", j);
			kfree(x[j]);
		}
	}

	for (int j = 7 ; j < 23 ; j++) {
		if (j % 5 == 1 || j % 5 == 2) {
			// print_info("malloc %d\n", j);
			uint32_t b = 0;
			switch (j% 3) {
			case 0:
				b = j / 5;
			case 1:
				b = 45;
			case 2:
				b = j * 11;
			};
			x[j] = kmalloc(b);
			ASSERT( x_size[j] == b);
			ASSERT( x_sizeno[j] == BlockHeader::bh(x[j])->sizeno);
		}
	}

	ASSERT( memcmp(&c, &bh[7]->ph()->count, sizeof(PH_count)) == 0);
	ASSERT( memcmp(&cu, &bh[7]->ph()->count_used, sizeof(PH_count)) == 0);

	/* hepsini sil */
	do {
		// print_info(">> free %d\n", i);
		kfree(x[i]);
	} while (--i > 0);
}


static void test3() {
	i = 0;
	for ( ; i < 30 ; ) {
		++i;
		int b = 0;
		switch (i % 3) {
		case 0:
			b = i / 5;
			break;
		case 1:
			b = 45;
			break;
		case 2:
			b = i * 11;
			break;
		};
		// print_info(">> malloc %d\n", i);
		x[i] = kmalloc(b);

		bh[i] = BlockHeader::bh(x[i]);
		x_size[i] = b;
		x_sizeno[i] = bh[i]->sizeno;

		kfree(x[i]);
		ASSERT( x[i] == kmalloc(b) );
	}

	c[0] = page_headers[0].count;
	c[1] = page_headers[1].count;
	cu[0] = page_headers[0].count_used;
	cu[1] = page_headers[1].count_used;

	for (int j = 3 ; j < 19 ; j++) {
		if (j % 5 == 2 || j % 5 == 3) {
			// print_info("free %d\n", j);
			kfree(x[j]);
		}
	}

	for (int j = 3 ; j < 19 ; j++) {
		if (j % 5 == 2 || j % 5 == 3) {
			// print_info("malloc %d\n", j);
			uint32_t b = 0;
			switch (j % 3) {
			case 0:
				b = j / 5;
				break;
			case 1:
				b = 45;
				break;
			case 2:
				b = j * 11;
				break;
			};
			x[j] = kmalloc(b);
			ASSERT( x_size[j] == b);
			ASSERT( x_sizeno[j] == BlockHeader::bh(x[j])->sizeno);
		}
	}

	ASSERT( memcmp(&c[0], &page_headers[0].count, sizeof(PH_count)) == 0);
	ASSERT( memcmp(&c[1], &page_headers[1].count, sizeof(PH_count)) == 0);
	ASSERT( memcmp(&cu[0], &page_headers[0].count_used, sizeof(PH_count)) == 0);
	ASSERT( memcmp(&cu[1], &page_headers[1].count_used, sizeof(PH_count)) == 0);

	// listele(&page_headers[0]);
	// listele(&page_headers[1]);

	for (int j = 3 ; j < 19 ; j++) {
		if (j % 5 == 1 || j % 5 == 2) {
			// print_info("free %d\n", j);
			kfree(x[j]);
		}
	}

	for (int j = 3 ; j < 19 ; j++) {
		if (j % 5 == 1 || j % 5 == 2) {
			// print_info("malloc %d\n", j);
			uint32_t b = 0;
			switch (j% 3) {
			case 0:
				b = j / 5;
				break;
			case 1:
				b = 45;
				break;
			case 2:
				b = j * 11;
				break;
			};
			x[j] = kmalloc(b);
			ASSERT( x_size[j] == b);
			ASSERT( x_sizeno[j] == BlockHeader::bh(x[j])->sizeno);
		}
	}

	// listele(&page_headers[0]);
	// listele(&page_headers[1]);

	ASSERT( memcmp(&c[0], &page_headers[0].count, sizeof(PH_count)) == 0);
	ASSERT( memcmp(&c[1], &page_headers[1].count, sizeof(PH_count)) == 0);
	// FIXME: bu testte asagidakilerin farkli olmasi hata mi bilmiyorum
	// (programda hatali, kernelde duzgun calisiyor)
	ASSERT( memcmp(&cu[0], &page_headers[0].count_used, sizeof(PH_count)) == 0);
	ASSERT( memcmp(&cu[1], &page_headers[1].count_used, sizeof(PH_count)) == 0);
}

static void test4() {
	/* test 3' devam */
	int baslangic = 1;
	int bitis = 30;

	for ( ; baslangic < bitis ; baslangic++) {
		for (int j = baslangic ; j < bitis ; j++) {
			if (j % 5 == 2 || j % 5 == 3) {
				// print_info("free %d\n", j);
				kfree(x[j]);
			}
		}

		for (int j = baslangic ; j < bitis ; j++) {
			if (j % 5 == 2 || j % 5 == 3) {
				// print_info("malloc %d\n", j);
				uint32_t b = 0;
				switch (j % 3) {
				case 0:
					b = j / 5;
					break;
				case 1:
					b = 45;
					break;
				case 2:
					b = j * 11;
					break;
				};
				x[j] = kmalloc(b);
				ASSERT( x_size[j] == b);
				ASSERT( x_sizeno[j] == BlockHeader::bh(x[j])->sizeno);
			}
		}

		ASSERT( memcmp(&c[0], &page_headers[0].count, sizeof(PH_count)) == 0);
		ASSERT( memcmp(&c[1], &page_headers[1].count, sizeof(PH_count)) == 0);
		// FIXME: bu testte asagidakilerin farkli olmasi hata mi bilmiyorum
		// (programda hatali, kernelde duzgun calisiyor)
		ASSERT( memcmp(&cu[0], &page_headers[0].count_used, sizeof(PH_count)) == 0);
		ASSERT( memcmp(&cu[1], &page_headers[1].count_used, sizeof(PH_count)) == 0);

	}

	for (baslangic = 1, bitis = 30 ; baslangic < bitis ; baslangic++) {
		// listele(&page_headers[0]);
		// listele(&page_headers[1]);

		for (int j = baslangic ; j < bitis ; j++) {
			// print_info("free %d\n", j);
			kfree(x[j]);
		}

		for (int j = baslangic ; j < bitis ; j++) {
			// print_info("malloc %d\n", j);
			uint32_t b = 0;
			switch (j % 3) {
			case 0:
				b = j / 5;
				break;
			case 1:
				b = 45;
				break;
			case 2:
				b = j * 11;
				break;
			};
			x[j] = kmalloc(b);
			ASSERT( x_size[j] == b);
			ASSERT( x_sizeno[j] == BlockHeader::bh(x[j])->sizeno);
		}

		// listele(&page_headers[0]);
		// listele(&page_headers[1]);

		// FIXME: bu hata mi? (0 ve 1 yer degistiriyor) (kernelde normal
		// calisiyor, test programinda hatali)
		compare_total_count();
		ASSERT( memcmp(&c[0], &page_headers[0].count, sizeof(PH_count)) == 0);
		ASSERT( memcmp(&c[1], &page_headers[1].count, sizeof(PH_count)) == 0);
		// FIXME: bu testte asagidakilerin farkli olmasi hata mi bilmiyorum
		// (programda hatali, kernelde duzgun calisiyor)
		ASSERT( memcmp(&cu[0], &page_headers[0].count_used, sizeof(PH_count)) == 0);
		ASSERT( memcmp(&cu[1], &page_headers[1].count_used, sizeof(PH_count)) == 0);
	}

	for (baslangic = 1, bitis = 30 ; baslangic < bitis ; bitis--) {
		for (int j = baslangic ; j < bitis ; j++) {
			// print_info("free %d\n", j);
			kfree(x[j]);
		}

		for (int j = baslangic ; j < bitis ; j++) {
			// print_info("malloc %d\n", j);
			uint32_t b = 0;
			switch (j % 3) {
			case 0:
				b = j / 5;
				break;
			case 1:
				b = 45;
				break;
			case 2:
				b = j * 11;
				break;
			};
			x[j] = kmalloc(b);
			ASSERT( x_size[j] == b);
			ASSERT( x_sizeno[j] == BlockHeader::bh(x[j])->sizeno);
		}

		// FIXME: degisimler oluyor fakat toplam parca sayisi ayni
		compare_total_count();
	}

	/* hepsini sil */
	i = 30;
	do {
		// print_info(">> free %d\n", i);
		kfree(x[i]);
	} while (--i > 0);

	// listele(&page_headers[0]);
	// listele(&page_headers[1]);

	ASSERT( memcmp(&page_headers[0], &page_headers[1], sizeof(page_headers[0])) == 0 );
	memset(&page_headers[0], 0, sizeof(page_headers[0]));
	ASSERT( memcmp(&page_headers[0], &page_headers[1], sizeof(page_headers[0])) == 0 );

	/* test 2 tmp page kullaniyor ikisi de bos olmali */
	extern uint32_t tmp_pages[];
	ASSERT( (tmp_pages[0] & 0x3) == 0);
}

static void compare_total_count() {
	for (int i = FIRST_SIZENO ; i <= LAST_SIZENO ; i++) {
		int c_ = c[0].get(i) + c[1].get(i);
		int ph = page_headers[0].count.get(i) + page_headers[1].count.get(i);
		ASSERT( c_ == ph );
	}
}
