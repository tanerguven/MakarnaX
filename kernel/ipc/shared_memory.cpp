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

#include <stdio.h>
#include  <types.h>
#include "../panic.h"

#include "../trap.h"
#include "../task.h"

// kmalloc.cpp
extern void *kmalloc(size_t size);
extern void kfree(void *v);
extern size_t kmalloc_size(size_t size);
//

/*
 * kaynaklar
 * http://linux.die.net/man/2/shmget
 */

struct HI_SharedMemId {
	static const ptr_t offset_struct;
	static const ptr_t offset_id;
};
typedef IdHashTable<struct SharedMemInfo, HI_SharedMemId> SharedMemIdHT_t;

struct HI_SharedMemKey {
	static const ptr_t offset_struct;
	static const ptr_t offset_id;
};
typedef IdHashTable<struct SharedMemInfo, HI_SharedMemKey> SharedMemKeyHT_t;

struct SharedMemInfo {
	key_t id;
	key_t key;
	size_t size;
	Page *page;
	SharedMem_Task_List_t task_list;
	SharedMemKeyHT_t::node_t key_hash_node;
	SharedMemIdHT_t::node_t id_hash_node;

	inline uint32_t attach_count()
		{ return task_list.size(); }

	inline void init() {
		task_list.init();
		key_hash_node.init();
		id_hash_node.init();
	}
};

static uint8_t mem_shm_id_ht[4096];
static SharedMemIdHT_t shm_id_ht;
static uint32_t next_shm_id = 1;

static uint8_t mem_shm_key_ht[4096];
static SharedMemKeyHT_t shm_key_ht;

void shm_init() {
	shm_id_ht.init(mem_shm_id_ht, sizeof(mem_shm_id_ht));
	shm_key_ht.init(mem_shm_key_ht, sizeof(mem_shm_key_ht));
}

// FIXME: hata icin return degerlerini duzenle
// TODO: flag
/* allocate shared memory */
void sys_shmget() {
	Trapframe *tf = task_curr->registers();
	key_t key = (key_t)get_param2(tf);
	size_t size = (size_t)get_param3(tf);
	int shmflg = (int)get_param4(tf);

	SharedMemInfo *shm = NULL;
	Page *p = NULL;

	if (size > 0x1000) {
		printf(">> shm max size: 4096 byte\n");
		return set_return(tf, -1);
	}

	if ((shmflg != 0) && (shmflg != 1)) {
		printf(">> shmflg not supported. Use [0, 1]\n");
		return set_return(tf, -1);
	}

	uint32_t eflags = eflags_read();
	cli();

	/* var olan shared memory */
	if (shmflg == 0) {
		shm = shm_key_ht.get(key);
		eflags_load(eflags);
		if (shm == NULL)
			return set_return(tf, -1);
		return set_return(tf, shm->id);
	}

	// shmflg == 1

	shm = shm_key_ht.get(key);
	if (shm != NULL)
		return set_return(tf, -1);

	int shmid = next_shm_id++;
	if (shmid > 1023) {
		printf(">> shm id kalmadi\n");
		return set_return(tf, -1);
	}

	shm = (SharedMemInfo*)kmalloc(sizeof(SharedMemInfo));
	if (shm == NULL)
		goto bad_sys_shmget;

	/* shared memoryde kullanilacak fiziksel bellek */
	if ( page_alloc(&p) < 0)
		goto bad_sys_shmget;
	p->refcount_inc();

	shm->init();
	shm->size = size;
	shm->id = shmid;
	shm->key = key;
	shm->page = p;

	shm_key_ht.put(&shm->key_hash_node);
	shm_id_ht.put(&shm->id_hash_node);

	eflags_load(eflags);
	return set_return(tf, shmid);

bad_sys_shmget:
	if (shm)
		kfree(shm);
	if (p)
		page_free(p);

	eflags_load(eflags);
	return set_return(tf, -1);
}

/* shared memory control */
void sys_shmctl() {
	Trapframe *tf = task_curr->registers();
	int shmid = (int)get_param2(tf);
	int cmd = (int)get_param3(tf);
	struct shmid_ds* buf = (struct shmid_ds*)get_param4(tf);

	printf(">> sys_shmctl\n");
	printf(">> shmid: %d\n", shmid);
	printf(">> cmd: %d\n", cmd);
	printf(">> buf: %08x\n", buf);

	return set_return(tf, -1);
}

/* attach shared memory to calling process addres space */
void sys_shmat() {
	Trapframe *tf = task_curr->registers();
	int shmid = (int)get_param2(tf);
	const void* shmaddr = (const void*)get_param3(tf);
	int shmflg = (int)get_param4(tf);

	PTE_t * pte = NULL;
	uint32_t va = -1;

	uint32_t eflags = eflags_read();
	cli();

	SharedMemInfo *shm = shm_id_ht.get(shmid);

	if (shm == NULL) {
		eflags_load(eflags);
		return set_return(tf, -1);
	}

	if (shmaddr != NULL) {
		printf(">> shmaddr not supported. Use NULL\n");
		eflags_load(eflags);
		return set_return(tf, -1);
	}

	if (shmflg != 0) {
		printf(">> shmflg not supported. Use 0\n");
		eflags_load(eflags);
		return set_return(tf, -1);
	}

	SharedMemDesc *desc = (SharedMemDesc*)kmalloc(sizeof(SharedMemDesc));
	if (desc == NULL)
		goto bad_sys_shmat;

	desc->init();

	/* shared memory icin sanal adres bul */
	for (va = MMAP_USER_SHARED_MEM_BASE ;
		 va < MMAP_USER_SHARED_MEM_TOP ;
		 va += 0x1000) {
		PTE_t *p = task_curr->pgdir.page_get_c(VA_t(va));
		if (!p->present) {
			pte = p;
			break;
		}
	}

	if (pte == NULL) {
		printf(">> shm: no virtual memory\n");
		goto bad_sys_shmat;
	}

	task_curr->pgdir.page_insert(shm->page, va,  PTE_P | PTE_U | PTE_W);
	task_curr->pgdir.count_shared++;

	desc->start = va2uaddr(va);
	desc->end = desc->start + shm->size;
	desc->info = shm;

	/* taskin shared memory listesine ekle */
	task_curr->shared_mem_list.push_back(&desc->list_node);
	/* shared memorynin task listesine ekle */
	desc->info->task_list.push_back(&desc->info_task_list_node);

	/* desc veriyapisi icin gecici kontrol */
	ASSERT(task_curr == desc->task());

	eflags_load(eflags);
	return set_return(tf, va2uaddr(va));

bad_sys_shmat:
	if (desc)
		kfree(desc);

	eflags_load(eflags);
	return set_return(tf, -1);
}

int detach_shared_mem(SharedMemDesc* d) {
	ASSERT(!(eflags_read() & FL_IF));
	for (uint32_t i = uaddr2va(d->start) ; i < uaddr2va(d->end) ; i+=0x1000) {
		int r = d->task()->pgdir.page_remove(i);
		if (r < 0)
			PANIC("shared memory kodlarinda hata olabilir");
		task_curr->pgdir.count_shared--;
		/* ref count 0 olamaz, shmget ile arttirildi */
		ASSERT( r > 0 );
	}

	d->list_node.__list->erase(&d->list_node);
	d->info_task_list_node.__list->erase(&d->info_task_list_node);

	return 0;
}

/* dettach shared memory from calling process addres space */
void sys_shmdt() {
	Trapframe *tf = task_curr->registers();
	const void* shmaddr = (const void*)get_param2(tf);

	uint32_t eflags = eflags_read();
	cli();

	SharedMemList_t::iterator i = task_curr->shared_mem_list.begin();
	SharedMemList_t::iterator end = task_curr->shared_mem_list.end();
	for ( ; i != end ; i++) {
		if (i->value()->start == (uint32_t)shmaddr) {
			break;
		}
	}
	if (i == end) {
		eflags_load(eflags);
		return set_return(tf, -1);
	}

	detach_shared_mem(i->value());
	eflags_load(eflags);

	return set_return(tf, 0);
}

int shm_fork(Task *child) {
	ASSERT(!(eflags_read() & FL_IF));
	child->shared_mem_list.init();

	SharedMemList_t::iterator i = task_curr->shared_mem_list.begin();
	SharedMemList_t::iterator end = task_curr->shared_mem_list.end();

	for ( ; i != end ; i++) {
		SharedMemDesc *desc = (SharedMemDesc*)kmalloc(sizeof(SharedMemDesc));
		if (desc == NULL)
			goto bad_shm_fork;
		desc->init();
		desc->start = i->value()->start;
		desc->end = i->value()->end;
		desc->info = i->value()->info;

		child->pgdir.link_pages(&task_curr->pgdir, MMAP_USER_SHARED_MEM_BASE,
								MMAP_USER_SHARED_MEM_TOP,
								PTE_P | PTE_U | PTE_W);

		/* child taskin shared memory listesine ekle */
		child->shared_mem_list.push_back(&desc->list_node);
		/* shared memorynin task listesine ekle */
		desc->info->task_list.push_back(&desc->info_task_list_node);
	}

	return 0;

bad_shm_fork:
	while (child->shared_mem_list.size() > 0) {
		kfree(child->shared_mem_list.front());
		child->shared_mem_list.pop_front();
	}
	return -1;
}

void shm_task_free(Task* t) {
	// printf(">> task_free_shared_memory\n");
	ASSERT(t == task_curr);

	while (t->shared_mem_list.size() > 0) {
		SharedMemDesc *d = t->shared_mem_list.front();
		detach_shared_mem(d);
		/* hatirlatma: detach_shared_mem listeden siliyor */
	}
	ASSERT(t->shared_mem_list.size() == 0);
}


uint32_t mem_shm() {
	uint32_t size = 0;
	for (int i = 0 ; i < shm_id_ht.__table_size ; i++) {
		SharedMemIdHT_t::node_t *n = shm_id_ht.__table[i];
		while (n != NULL) {
			// size += n->value()->size;
			size += 0x1000;
			size += kmalloc_size(sizeof(SharedMemInfo));
			size += n->value()->task_list.size() * kmalloc_size(sizeof(SharedMemDesc));
			n = n->__next;
		}
	}
	return size >> 10;
}

const ptr_t LI_SharedMem::offset_node_value =
	(ptr_t)offsetof(SharedMemDesc,list_node);
const ptr_t LI_SharedMem_Task::offset_node_value =
	(ptr_t)offsetof(SharedMemDesc,info_task_list_node);

const ptr_t HI_SharedMemId::offset_struct =
	(ptr_t)offsetof(SharedMemInfo,id_hash_node);
const ptr_t HI_SharedMemId::offset_id =
	(ptr_t)(offsetof(SharedMemInfo,id_hash_node) - offsetof(SharedMemInfo,id));

const ptr_t HI_SharedMemKey::offset_struct =
	(ptr_t)offsetof(SharedMemInfo,key_hash_node);
const ptr_t HI_SharedMemKey::offset_id =
	(ptr_t)(offsetof(SharedMemInfo,key_hash_node) - offsetof(SharedMemInfo,key));
