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
 * WMC List
 */

#undef NODE_LIST_LINK

#ifndef USE_LIST_1
#define NODE_LIST_LINK
#endif

template <class T, class O>
struct ListIterator;

template <class T, class O>
struct List;

template <class T, class O>
struct ListNode {
	ListNode* __prev;
	ListNode* __next;
#ifdef NODE_LIST_LINK
	List<T, O>* __list;
#endif

	inline void init() {
#ifdef NODE_LIST_LINK
		__list = NULL;
#endif
		__next = __prev = NULL;
	}

	inline T* value() const {
		return (T*)((wmc_ptr_t)this - O::offset_node_value);
	}

	inline const ListNode* next() const { return __next; }
	inline const ListNode* prev() const { return __prev; }

	inline bool is_free() const {
#ifdef NODE_LIST_LINK
		return __list == NULL;
#else
		return (__next == NULL) && (__prev == NULL);
#endif
	}

	inline void set_free() {
#ifdef NODE_LIST_LINK
		__list = NULL;
#else
		__next = __prev = NULL;
#endif
	}
	inline ListIterator<T, O> get_iterator() {
		return this;
	}
};

template <class T, class O>
struct ListIterator {
	ListNode<T, O> *__i;

	inline ListIterator() {}

	inline ListIterator(ListNode<T, O>* n)
		: __i(n) {}

	inline void operator++(int) { __i = __i->__next; }
	inline void operator--(int) { __i = __i->__prev; }

	inline bool operator==(const ListIterator &o)
		const { return __i == o.__i; }

	inline bool operator!=(const ListIterator &o)
		const { return ! operator==(o); }

#ifdef NODE_LIST_LINK
	/* noddan listeye ulasilabilen listelerde + ve - operatorleri kullanilabilir */
	/** liste bittiginde end dondurur. */
	inline ListIterator operator-(unsigned int n) {
		ListIterator<T, O> tmp = *this;
		for (; n > 0; n--) {
			tmp--;
			if (tmp.__i == &tmp->__list->__)
				break;
		}
		return tmp;
	}
	/** liste bittiginde end dondurur. */
	inline ListIterator operator+(unsigned int n) {
		ListIterator<T, O> tmp = *this;
		for (; n > 0; n--) {
			tmp++;
			if (tmp.__i == &tmp->__list->__)
				break;
		}
		return tmp;
	}
#endif

	inline ListNode<T, O>* operator*() const { return __i; }
	inline ListNode<T, O>* operator->() { return operator*(); }
};

template <class T, class O>
struct List {
	typedef ListIterator<T, O> iterator;
	typedef ListNode<T, O> node_t;

	unsigned int __size;
	/* hayali eleman */
	node_t __;

	inline void init() {
		__size = 0;
		__.__next = &__;
		__.__prev = &__;
#ifdef NODE_LIST_LINK
		__.__list = this;
#endif
	}

	inline iterator insert(iterator pos, node_t* n) {
		/* listede bulunan eleman tekrar eklenemez */
		if (!n->is_free())
			return error();
#ifdef NODE_LIST_LINK
		n->__list = this;
#endif
		n->__next = *pos;
		n->__prev = pos->__prev;
		n->__prev->__next = n;
		pos->__prev = n;

		__size++;
		return n;
	}

	inline iterator erase(iterator pos) {
		/* hayali eleman silinemez */
		if (*pos == &__)
			return error();
/*
 * Baska listedeki bir eleman silinmeye calisilirsa iki listenin de yapisi
 * bozulur. Kullanici hatalarini engellemek icin bir kontrol.
 * TODO: makroya gore etkinlestir
 */
#if 1
		if (pos->__list != this)
			return error();
#endif
		pos->__prev->__next = pos->__next;
		pos->__next->__prev = pos->__prev;
		node_t *next = pos->__next;

		pos->set_free();
		__size--;
		return next;
	}

	inline unsigned int size() const { return __size; }
	inline iterator begin() { return __.__next; }
	inline iterator end() { return &__; }
	inline T* front() { return __.__next->value(); }
	inline T* back() { return __.__prev->value(); }
	static inline const iterator error()
		{ return NULL; }

	inline bool push_front(node_t* n)
		{ return (insert(begin(), n) != error()); }
	inline bool push_back(node_t* n)
		{ return (insert(end(), n) != error()); }
	inline bool pop_front()
		{ return error() != erase(begin()); }
	inline bool pop_back()
		{ return error() != erase(end()-1); }
};

#undef NODE_LIST_LINK
