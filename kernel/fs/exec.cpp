/*
 * Copyright (C) 2012 Taner Guven <tanerguven@gmail.com>
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

#include "../elf.h"
#include "../task.h"
#include "../fs/vfs.h"

#include <types.h>

int init_user_stack(char *const argv[], addr_t *user_esp) {
	int r;
	addr_t esp = (addr_t)va2kaddr(MMAP_USER_STACK_TOP);
	addr_t arg_ptr[20];
	int i, argc;

	/* process icin stack alani olustur */
	r = task_curr->pgdir.page_alloc_insert(MMAP_USER_STACK_TOP - 0x1000,
											 PTE_P | PTE_U | PTE_W);
	if (r < 0)
		return r;
	task_curr->pgdir.count_stack = 1;

	/* argv'yi stack'e bas */
	for (i = 0 ; argv[i] != NULL ; i++) {
		esp -= strlen(argv[i]) + 1;
		strcpy((char*)esp, argv[i]);
		arg_ptr[i] = esp;
	}
	argc = i;
	while ( --i > -1 ) {
		esp -= sizeof(addr_t);
		*(addr_t*)esp = kaddr2uaddr(arg_ptr[i]);
	}

	/* argv, argc, eip degerlerini stacke bas */
	((addr_t*)esp)[-1] = kaddr2uaddr(esp); /* argv */
	((addr_t*)esp)[-2] = argc; /* argc */
	((addr_t*)esp)[-3] = va2uaddr(0xffffffff); /* return eip */
	esp -= sizeof(addr_t) * 3;

	*user_esp = kaddr2uaddr(esp);

	return 0;
}

int load_program(File* f, Elf32_Ehdr *header, uint32_t *last_addr) {
	int r;
	Elf32_Phdr ph;
	uint32_t old_pos;
	*last_addr = 0;

	/* ilk program header'i (ph), binary'nin baslangici + ph offset */
	f->fpos = header->phoff;
	for (int i = 0 ; i < header->phnum ; i++) {
		ASSERT( do_read(f, (char*)&ph, sizeof(Elf32_Phdr)) == sizeof(Elf32_Phdr) );
		if (ph.type == Elf32_Phdr::Type_LOAD) {
			/* programin bulunacagi bellek araligini ayir */
			r = task_curr->pgdir.segment_alloc(uaddr2va(ph.vaddr), ph.memsz,
												 PTE_P | PTE_U | PTE_W);
			if (r < 0)
				return r;

			/* programin bitis adresi, brk icin gerekli */
			if (roundUp(ph.vaddr + ph.memsz) > *last_addr)
				*last_addr = roundUp(ph.vaddr + ph.memsz);

			task_curr->pgdir.count_program += roundUp(ph.memsz) / 0x1000;

			/* dosyadan bellekte bulunacagi yere oku */
			old_pos = f->fpos;
			f->fpos = ph.offset;
			r = do_read(f, (char*)uaddr2kaddr(ph.vaddr), ph.memsz);
			ASSERT((uint32_t)r == ph.memsz);
			f->fpos = old_pos;

			/* programin sonunda kalan alani sifirla */
			if (ph.memsz > ph.filesz)
				memset((void*)(uaddr2kaddr(ph.vaddr) + ph.filesz),
					   0, ph.memsz - ph.filesz);
		}
	}

	return 0;
}

int do_execve(const char *path, char *const argv[]) {
	int r;
	File *f;
	Trapframe registers;
	Elf32_Ehdr header;
	addr_t last_addr;

	cli();

	/* dosyayi ac ve dosya headerini oku */
	r = do_open(&f, path);
	if (r < 0)
		return r;
	r = do_read(f, (char*)&header, sizeof(header));
	if (r < 0)
		return r;

	/* prosesin baslangictaki register degerleri */
	memset(&registers, 0, sizeof(registers));
	registers.eip = header.entry; // programin baslangic adresi
	registers.ds = registers.es = registers.ss = GD_UD | 3;
	registers.cs = GD_UT | 3;
	registers.eflags = FL_IF; // interruplar aktif

	/* stack'i olustur, argv'yi stacke bas ve registerlara esp degerini ata */
	r = init_user_stack(argv, &registers.esp);
	if (r < 0)
		return r;

	/* programi bellege (prosesin adres uzayina) yukle */
	r = load_program(f, &header, &last_addr);
	if (r < 0)
		return r;

	do_close(f);

	/* heap alani */
	task_curr->pgdir.start_brk = task_curr->pgdir.end_brk = last_addr;

	/* kullanici moduna, programin baslangicina atla */
	asm volatile(
		"movl %0, %%esp\n\t"
		"jmp trapret"
		:: "r"(&registers)
		);

	PANIC("--");
	return -1;
}

/*
 * FIXME: gecici cozum
 * execve ile gelen degiskenleri kopyalamak icin degiskenler
 */
char _a[16][256];
char _path[MAX_PATH_SIZE];
char *_argv[16];

// FIXME: parametre kontrolunu duzelt
asmlink void sys_execve() {
	int i, r;
	Trapframe *tf = task_curr->registers();

	cli();

	const char *path = (const char*)uaddr2kaddr(get_param1(tf));
	char *const *argv = (char* const*)uaddr2kaddr(get_param2(tf));

	/* kullanicidan gelen parametreleri kopyala */
	strncpy(_path, path, MAX_PATH_SIZE);
	i = 0;
	do {
		_argv[i] = _a[i];
		strncpy(_a[i], (char*)uaddr2kaddr((uint32_t)argv[i]), MAX_DIR_ENTRY_SIZE);
		i++;
	} while(argv[i] != NULL);
	_argv[i] = NULL;

	r = do_execve(_path, _argv);

	return set_return(tf, r);
}
