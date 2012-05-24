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
#include <asm/x86.h>

#include "test_programs.h"
#include "memory/physical.h"

extern void init_console();
extern int memory_init();
extern void init_traps();
extern void task_init();
extern void schedule_init();
extern void run_first_task();
extern void picirq_init();
extern void timer_init();
extern void ipc_init();
extern struct DirEntry* init_vfs();
extern void init_kernel_task(struct DirEntry* root);

extern uint32_t free_memory_start;

extern int do_fork();
extern int do_execve(const char *path, char *const argv[]);

void kernel_task();
void init_task();
void run_kernel_test(const char* name);

#define STR(x) #x
#define TO_STR(x) STR(x)
#ifdef KTEST
# define __KERNEL_TEST_FUNCTION TO_STR(KTEST)
#endif

// gecici
#include "fs/vfs.h"
int do_mknod(const char* pathname, FileMode mode, int dev);

asmlink int main() {
	struct DirEntry *root_dir;
	int r;

	task_init();

	init_console();
	print_info("\n");

#ifdef __KERNEL_TEST_FUNCTION
	run_kernel_test(__KERNEL_TEST_FUNCTION);
#endif

	memory_init();
	init_traps();

	schedule_init();
	ipc_init();

	picirq_init();
	timer_init();

	root_dir = init_vfs();

	free_memory_start = mem_free();

	print_info("init_kernel_task\n");
	init_kernel_task(root_dir);

	/* stdout dosyasi denemesi: */
	FileMode fm;
	fm.type = FileMode::FT_chrdev;
	fm.write = 1;
	r = do_mknod("/stdout", fm, 11);
	ASSERT(r == 0);
	/* */

	r = do_fork();
	ASSERT(r > -1);
	sti();
	if (r == 0) {
		init_task();
		PANIC("init task sonlandi\n");
	}
	kernel_task();
	PANIC("kernel task sonlandi");

	while (1);
}

void kernel_task() {
	print_info(">> kernel_task started\n");
	while (1) {
		sti();
		asm("hlt");
		cli();
	}
}

void init_task() {
	int r;
	char* argv[1] = { NULL };

	print_info(">> init_task started\n");

	r = do_execve("/bin/init", argv);
	ASSERT(r == 0);
}


/*********************************
 * Kernel testleri
 *********************************/

extern void kernel_test_fs();

struct KernelTest {
	const char *name;
	void (*function)();
};

const KernelTest kernel_test[] = {
	{ "fs", kernel_test_fs },
	{ NULL, NULL }
};

const KernelTest *get_kernel_test(const char *name) {
	const KernelTest *i;

	for (i = kernel_test ; i->name != NULL ; i++) {
		if (strcmp(i->name, name) == 0)
			return i;
	}
	return NULL;
}

void run_kernel_test(const char *name) {
	const KernelTest* ktest = get_kernel_test(name);
	if (ktest == NULL)
		PANIC("ktest not found");
	ktest->function();
	PANIC("kernel test finished");
}

/******************************
 * Test programlari
 ******************************/

const TestProgram test_programs[] = {
	{ "yield", &TEST_PROGRAM(yield), &TEST_PROGRAM_END(yield) },
	{ "hello", &TEST_PROGRAM(hello), &TEST_PROGRAM_END(hello) },
	{ "divide_error", &TEST_PROGRAM(divide_error), &TEST_PROGRAM_END(divide_error) },
	{ "forktest", &TEST_PROGRAM(forktest), &TEST_PROGRAM_END(forktest) },
	{ "dongu", &TEST_PROGRAM(dongu), &TEST_PROGRAM_END(dongu) },
	{ "sys_dongu", &TEST_PROGRAM(sys_dongu), &TEST_PROGRAM_END(sys_dongu) },
	{ "signaltest", &TEST_PROGRAM(signaltest), &TEST_PROGRAM_END(signaltest) },
	{ "ipctest", &TEST_PROGRAM(ipctest), &TEST_PROGRAM_END(ipctest) },
	{ "processmemtest", &TEST_PROGRAM(processmemtest),
	  &TEST_PROGRAM_END(processmemtest) },
	{ "kill", &TEST_PROGRAM(kill), &TEST_PROGRAM_END(kill) },
	{ "fs", &TEST_PROGRAM(fs), &TEST_PROGRAM_END(fs) },
	{ "init", &TEST_PROGRAM(init), &TEST_PROGRAM_END(init) },
	{ "hello_newlib", &TEST_PROGRAM(hello_newlib), &TEST_PROGRAM_END(hello_newlib) },
};
const size_t nr_test_programs = sizeof(test_programs)/sizeof(test_programs[0]);

const TestProgram *test_program(const char *name) {
	for (unsigned int i = 0 ; i < nr_test_programs ; i++) {
		if ( strcmp(test_programs[i].name, name) == 0 )
			return &test_programs[i];
	}
	return NULL;
}
