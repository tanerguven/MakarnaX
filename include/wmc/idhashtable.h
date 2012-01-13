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

#ifndef _WMC_ID_HASH_TABLE_H_
#define _WMC_ID_HASH_TABLE_H_

template <class T, class O>
struct IdHashTableNode {
	IdHashTableNode* __prev;
	IdHashTableNode* __next;

	inline void init()
		{ __prev = __next = NULL; }

	inline bool is_free()
		{ return (__prev == NULL) && (__next == NULL); }

	inline T* value()
		{ return (T*)((char*)this - O::offset_struct); }

	inline unsigned int id()
		{ return *(unsigned int*)((char*)this - O::offset_id); }
};


template <class T, class O>
struct IdHashTable {
	typedef IdHashTableNode<T, O> node_t;

	node_t** __table;
	size_t __table_size;
	size_t __count;

	inline void init(void* table_memory, size_t memory_size) {
		__count = 0;
		__table = (node_t**)table_memory;
		this->__table_size = memory_size/sizeof(node_t*);
		for (unsigned int i = 0 ; i < __table_size ; i++)
			__table[i] = NULL;
	}

	inline int put(node_t* n);
	inline T* get(unsigned int id);
	inline int remove(node_t* n);

	inline size_t table_size()
		{ return __table_size; }

	inline size_t count()
		{ return __count; }
};


template <class T, class  O>
inline int IdHashTable<T, O>::put(node_t* n) {
	if (!n->is_free())
		return -1;

	unsigned int hash = n->id() % __table_size;
	node_t *i = __table[hash];

#if 1
	// TODO: Burasi makro ile kapatilabilir olmali, id cakisma ihtimali yoksa
	// kontrole gerek yok
	/* ayni id var mi? listeyi tara, ayni id'ye sahip elemanlar olamaz */
	while (i != NULL) {
		if (i->id() == n->id())
			return -1;
		i = i->__next;
	}
	i = __table[hash];
#endif
	if (i != NULL) {
		/* eleman varsa ilk elemanin oncesine ekle */
		n->__next = i;
	    i->__prev = n;
	} else {
		// FIXME: free oldugunu anlayabilmemiz icin
		n->__prev = (node_t*)0xffffffff;
	}
	__table[hash] = n;
	__count++;

	return 0;
}

template <class T, class O>
inline T* IdHashTable<T, O>::get(unsigned int id) {
	unsigned int hash = id % __table_size;
	node_t *i = __table[hash];

	while (i != NULL) {
		if (i->id() == id)
			break;
		i = i->__next;
	}

	if (i == NULL)
		return NULL;

	return i->value();
}


template <class T, class O>
inline int IdHashTable<T, O>::remove(node_t* n) {
	if (n->__prev) {
		n->__prev->__next = n->__next;
	} else {
		/* ilk elemansa */
		unsigned int hash = n->id() % __table_size;
		/* silinmeye calisilan eleman baska bir hash tablosunun elemani ise */
		if (__table[hash] != n)
			return -1;
		__table[hash] = n->__next;
	}
	if (n->__next)
		n->__next->__prev = n->__prev;

	n->__prev = n->__next = NULL;
	__count--;

	return 0;
}


#define define_id_hash(type_t,id_hash_t)\
struct HI_##id_hash_t {\
	static const ptr_t offset_struct;\
	static const ptr_t offset_id;\
};\
typedef IdHashTable<type_t, HI_##id_hash_t> id_hash_t;

#define set_id_hash_offset(type_t,id_hash_t,off_str,off_id)\
const ptr_t HI_##id_hash_t::offset_struct = (ptr_t)offsetof(type_t,off_str);\
const ptr_t HI_##id_hash_t::offset_id = (ptr_t)(offsetof(type_t,off_str) - offsetof(type_t,off_id));

#endif /* _WMC_ID_HASH_TABLE_H_ */
