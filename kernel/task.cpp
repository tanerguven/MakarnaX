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

#include <types.h>
#include <asm/x86.h>

#include "task.h"
#include "elf.h"

#include "sched.h"
#include "time.h"

#include <string.h>
#include <stdio.h>
using namespace std;

#include "kernel.h"

// signal.cpp
extern void free_sigstack();

// fs/vfs.cpp
extern struct File* dup_file(struct File *src);

// fs/file.cpp
void task_curr_free_files();

// boot.asm
extern "C" void *__boot_stack;

// memory/virtual.cpp
extern PageDirInfo kernel_dir;

/*
 * TODO: signal almis bir process, signal fonksiyonunda fork yaparsa ?
 * TODO: zombie tasklar icin bir cozum
 */

void switch_to_task(Task *newtask) __attribute__ ((noinline));
int do_fork();

/* liste tipleri icin nodlarin offset degerleri */
set_list_offset(struct Task, AlarmList_t, alarm_list_node);
set_list_offset(struct Task, ChildList_t, childlist_node);
set_list_offset(struct Task, TaskList_t, list_node);
set_id_hash_offset(struct Task, TaskIdHashTable_t, id_hash_node, id);

TaskIdHashTable_t task_id_ht;
uint8_t mem_task_id_ht[4096];
int next_task_id = 0;

TaskList_t task_zombie_list;
Task *task_curr;

/** task için sanal bellek oluşturur */
static int task_setup_vm(Task *t, PageDirInfo *parent_pgdir) {
	ASSERT(!(eflags_read() & FL_IF));

	int err;
	uint32_t va;
	Page *p;

	memset(&t->pgdir, 0, sizeof(t->pgdir));

	/* page directory yapisi icin bellek al */
	if ((err = tmp_page_alloc_map(&p, &va, PTE_P | PTE_W)) < 0)
		return err;
	t->pgdir.pgdir_pa = p->addr();
	t->pgdir.pgdir = (PageDirectory*)va2kaddr(va);
	memset(t->pgdir.pgdir, 0, 0x1000);
	t->pgdir.count_kernel++;

	/* page table listesi icin bellek al */
	if ((err = tmp_page_alloc_map(&p, &va, PTE_P | PTE_W)) < 0)
		return err;
	t->pgdir.pgtables = (PageTable**)va2kaddr(va);
	memset(t->pgdir.pgtables, 0, 0x1000);
	t->pgdir.count_kernel++;

	/* Kernel adres uzayini dogrudan task adres uzayina bagla */
	err = t->pgdir.link_pgtables(parent_pgdir, MMAP_KERNEL_BASE, MMAP_KERNEL_STACK_BASE);
	ASSERT(err == 0);

	return 0;
}

static void task_delete_vm(Task *t) {
	ASSERT(!(eflags_read() & FL_IF));

	/* task baska taski silemez, killemek icin signal gondermeli */
	ASSERT(t != task_curr);
	uint32_t va;

	/* free pgdir */
	va = (uint32_t)t->pgdir.pgdir;
	if (va > 0) {
		ASSERT( tmp_page_free(kaddr2va(va)) == 0);
		t->pgdir.count_kernel--;
	}

	/* free pgtables */
	va = (uint32_t)t->pgdir.pgtables;
	if (va > 0) {
		ASSERT( tmp_page_free(kaddr2va(va)) == 0);
		t->pgdir.count_kernel--;
	}

	ASSERT(t->pgdir.count_user == 0);
	ASSERT(t->pgdir.count_kernel == 0);
}

void task_free_kernel_stack(Task* t) {
	ASSERT(!(eflags_read() & FL_IF));
	uint32_t va;
	VA_t va_kernel_stack(MMAP_KERNEL_STACK_BASE);

	t->pgdir.segment_free(MMAP_KERNEL_STACK_BASE, MMAP_KERNEL_STACK_SIZE);

	/* free kernel stacks pgtable */
	va = (uint32_t)t->pgdir.pgtables[va_kernel_stack.pdx];
	if (va > 0) {
		ASSERT( tmp_page_free(kaddr2va(va)) == 0);
		t->pgdir.count_kernel--;
	}

	free_sigstack();
}

void task_free_vm_user(Task* t) {
	ASSERT(!(eflags_read() & FL_IF));

	/*  user alanindaki butun pageleri serbest birak */
	for (uint32_t pde_no = VA_t(MMAP_USER_BASE).pdx ;
		 pde_no < VA_t(MMAP_USER_TOP).pdx ; pde_no++) {
		/* kullanilmayan pagetablelari atla */
		if (!t->pgdir.pgdir->e[pde_no].present)
			continue;
		/* kullanilan butun pageleri serbest birak */
		for (uint32_t pte_no = 0 ; pte_no < 1024 ; pte_no++) {
			ASSERT(t->pgdir.pgtables[pde_no]);
			VA_t va(pde_no, pte_no);
			if (t->pgdir.pgtables[pde_no]->e[pte_no].present) {
                /* simdilik page paylasimi yok, silince refcount=0 olmali */
				int i = t->pgdir.page_remove(va, 0);
				ASSERT(i == 0);
			}
		}
		/* free pages page table */
		t->pgdir.pde_free(pde_no);
	}

	cr3_reload();

	t->pgdir.count_stack = 0;
	t->pgdir.count_program = 0;
	t->pgdir.count_shared = 0;
}

void task_free(Task *t) {
	ASSERT(!(eflags_read() & FL_IF));
	ASSERT(t == task_curr);

	ipc_task_free(t);
	task_curr_free_files();

	uint32_t count_brk = (t->pgdir.end_brk - t->pgdir.start_brk) / 0x1000;
	/* code, heap ve stack alanlari toplami user alanina esit olmali */
	ASSERT(t->pgdir.count_stack + t->pgdir.count_program + count_brk ==
		   t->pgdir.count_user);
	task_free_vm_user(t);
	ASSERT(t->pgdir.count_user == 0);

	/*
	 * kernel stack ve pagedir daha sonra zombie list uzerinden silinecek.
	 * task structini bulundugu listeden cikar, zombie olarak isaretle ve
	 * task_zombie_list'e ekle.
	 */
	t->list_node.__list->erase(t->list_node.get_iterator());
	t->state = Task::State_zombie;
	task_zombie_list.push_back(&t->list_node);

	if (t->alarm_list_node.__list)
		t->alarm_list_node.__list->erase(&t->alarm_list_node);
}

void free_zombie_tasks() {
	ASSERT(!(eflags_read() & FL_IF));

	while (task_zombie_list.size() > 0) {
		Task *t = task_zombie_list.front();
		if (t == task_curr)
			return;
		task_zombie_list.pop_front();
		task_free_kernel_stack(t);
		task_delete_vm(t);
		t->state = Task::State_not_runnable;;
		t->free = 1;
		kfree(t);
	}
}

void task_init() {
	task_id_ht.init(mem_task_id_ht, sizeof(mem_task_id_ht));
	task_zombie_list.init();

	printf(">> task_init OK\n");
}

asmlink void sys_getpid(void) {
	ASSERT(task_curr);
	return set_return(task_curr->registers(), task_curr->id);
}

asmlink void sys_fork() {
	int pid;

	uint32_t eflags = eflags_read();
	cli();

	pid = do_fork();

	eflags_load(eflags);
	return set_return(task_curr->registers(), pid);
}

// FIXME: uygun yere tasi
void move_stack(uint32_t ka_curr, uint32_t ka_new, uint32_t size) {
	memcpy((void*)ka_new,
		   (void*)ka_curr, size);
	uint32_t esp; read_reg(%esp, esp);
	load_reg(%esp, ka_new + esp - ka_curr);
	uint32_t ebp; read_reg(%ebp, ebp);
	load_reg(%ebp, ka_new + ebp - ka_curr);
}

inline void set_task_id(Task* t) {
	ASSERT(!(eflags_read() & FL_IF));

	t->id = next_task_id++;
	ASSERT( task_id_ht.put(&t->id_hash_node) == 0);
}

/* ilk prosesi (proses 0, kernel task) olusturur */
void init_kernel_task(struct DirEntry* root) {
	int r;

	ASSERT(!(eflags_read() & FL_IF));

	task_curr = (Task*)kmalloc(sizeof(Task));
	ASSERT( task_curr );

	memset(task_curr, 0, sizeof(Task));
	task_curr->init();
	task_curr->shared_mem_list.init();
	task_curr->priority = 3;
	task_curr->pgdir = kernel_dir;
	task_curr->root = task_curr->pwd = root;

	/* task icin kernel stack alani olustur */
	r = task_curr->pgdir.segment_alloc(MMAP_KERNEL_STACK_BASE,
									   MMAP_KERNEL_STACK_SIZE,
									   PTE_P | PTE_W);
	ASSERT( r == 0 );
	move_stack((uint32_t)&__boot_stack + (0x4000-MMAP_KERNEL_STACK_SIZE),
			   va2kaddr(MMAP_KERNEL_STACK_BASE), MMAP_KERNEL_STACK_SIZE);

	/* process id ata ve runnable listesine ekle */
	set_task_id(task_curr);
	add_to_runnable_list(task_curr);

	printf(">> task %d created\n", task_curr->id);
}

// TODO: fork kurallari duzenlenmeli (neler kopyalanacak, neler kopyalanmayacak?)
int do_fork() {
	/* debug only */
	uint32_t mem_before_setup_vm = 0;
	uint32_t mem_before_copy_pages = 0;
	uint32_t mem_before_kernel_stack = 0;
	/* */
	int r;
	Task *t;
	uint32_t eip;
	int e = 0; // error (bad_fork_* icin)
	uint32_t eflags;

	ASSERT(!(eflags_read() & FL_IF));

	t = (Task*)kmalloc(sizeof(Task));
	if (!t)
		goto bad_fork_task_alloc;

	memcpy(t, task_curr, sizeof(Task));
	t->init();
	t->parent = task_curr;
	t->state = Task::State_not_runnable;

	/* -- */
	mem_before_setup_vm = mem_free();
	r = task_setup_vm(t, &task_curr->pgdir);
	if (r < 0)
		goto bad_fork_setup_vm;

	/* user adres uzayini kopyala */
	mem_before_copy_pages = mem_free();
	r = t->pgdir.copy_pages(&task_curr->pgdir, MMAP_USER_BASE,
							MMAP_USER_SHARED_MEM_BASE);
	if (r < 0)
		goto bad_fork_copy_vm_user;
	/* shared memory kismini kopyalama, shm_fork fonksiyonu kopyalayacak */
	r = t->pgdir.copy_pages(&task_curr->pgdir, MMAP_USER_SHARED_MEM_TOP,
							MMAP_USER_TOP);
	if (r < 0)
		goto bad_fork_copy_vm_user;
	t->pgdir.count_program = task_curr->pgdir.count_program;
	t->pgdir.count_stack = task_curr->pgdir.count_stack;
	t->pgdir.start_brk = task_curr->pgdir.start_brk;
	t->pgdir.end_brk = task_curr->pgdir.end_brk;
	/* */

	/* ipc veriyapilari icin, ipc_fork */
	r = ipc_fork(t);
	if (r < 0)
		goto bad_fork_ipc;

	/* dosya bilgilerini kopyala */
	for (int i = 0 ; i < TASK_MAX_FILE_NR ; i++) {
		if (task_curr->files[i])
			t->files[i] = dup_file(task_curr->files[i]);
		else
			t->files[i] = NULL;
	}

	/* kernel stackini kopyala */
	mem_before_kernel_stack = mem_free();
	r = t->pgdir.copy_pages(&task_curr->pgdir, MMAP_KERNEL_STACK_BASE, MMAP_KERNEL_STACK_TOP);
	if (r < 0)
		goto bad_fork_copy_kernel_stack;

	/* burasi 2 kere calisiyor */
	eip = read_eip();
	if (eip == 1)
		return 0;

	/* child prosesin register bilgileri */
	t->k_eip = eip;
	read_reg(%esp, t->k_esp);
	read_reg(%ebp, t->k_ebp);

	/* child prosesin baslangic zamani */
	t->time_start = jiffies;

	eflags = eflags_read();
	cli();

	/* child listesine ekle */
	ASSERT( task_curr->childs.push_back(&t->childlist_node) );
	/* process id ata ve runnable listesine ekle */
	set_task_id(t);
	add_to_runnable_list(t);

	eflags_load(eflags);

	return t->id;

bad_fork_copy_kernel_stack:
	if (e++ == 0)
		printf("!! bad_fork_copy_kernel_stack\n");
	task_free_kernel_stack(t);
	ASSERT(mem_free() == mem_before_kernel_stack);
bad_fork_ipc:
	// TODO: --
bad_fork_copy_vm_user:
	if (e++ == 0)
		printf("!! bad_fork_copy_vm_user\n");
	task_free_vm_user(t);
	ASSERT(mem_free() == mem_before_copy_pages);
bad_fork_setup_vm:
	if (e++ == 0)
		printf("!! bad_fork_setup_vm\n");
	task_delete_vm(t);
	ASSERT(mem_free() == mem_before_setup_vm);
	kfree(t);
	t = NULL;
bad_fork_task_alloc:
	if (e++ == 0)
		printf("!! bad_fork_task_alloc\n");
	ASSERT(t == NULL);
	return -1;
}

/* bu fonksiyon calismadan once tum registerlar kayedilmeli */
void switch_to_task(Task *newtask) {
	CLOBBERED_REGISTERS_ALL();

	// printf(">> [%d] switch to %d\n", task_curr->id, newtask->id);

	ASSERT(!(eflags_read() & FL_IF));
	ASSERT(task_curr);

	/* eski proses calismissa */
	if (task_curr->k_eip == 0) {
		/* stack bilgilerini kaydediyoruz */
		read_reg(%esp, task_curr->k_esp);
		read_reg(%ebp, task_curr->k_ebp);
	}

	task_curr = newtask;
	task_curr->run_count++;

/*
 * bir adrese atlanarak task switch yapma:
 * sadece yeni proses ilk kez calistiginda kullaniliyor
 */
	if (task_curr->k_eip) {
		asm volatile(
			"movl (%0), %%eax\n\t"
			"movl $0, (%0)\n\t" // k_eip = 0
			"movl %1, %%esp\n\t" // %esp = k_esp
			"movl %2, %%ebp\n\t" // %ebp = k_ebp
			"movl %3, %%cr3\n\t" // %cr3 = pgdir.pgdir_pa
			"pushl $1\n\t" // read_eip() icin return degeri 1
			"pushl %%eax\n\t" // ret ile yuklenecek program counter (k_eip)
			"ret"
			:
			: "r"(&task_curr->k_eip),
			  "r"(task_curr->k_esp),
			  "r"(task_curr->k_ebp),
			  "r"(task_curr->pgdir.pgdir_pa)
			: "eax"
			);
	}

/*
 * program counter oldugu yerden devam ederek task switch yapma:
 * standart durumlarda bu yontem kullanilir
 * sadece yeni process'in page_directory'si ve stack'i yukleniyor
 */
	asm volatile(
		"movl %0, %%cr3\n\t" // %cr3 = pgdir.pgdir_pa
		"movl %1, %%esp\n\t" // %esp = k_esp
		"movl %2, %%ebp\n\t" // %ebp = k_ebp
		:
		: "r"(task_curr->pgdir.pgdir_pa),
		  "r"(task_curr->k_esp),
		  "r"(task_curr->k_ebp)
		:
		);

	/* bu satirdan yeni process ile devam ediliyor */
}
