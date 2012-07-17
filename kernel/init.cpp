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
extern void init_fs(struct dirent **root);
extern void init_kernel_task(struct dirent* root);

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
// #include "fs/vfs.h"
// int do_mknod(const char* pathname, FileMode mode, int dev);

asmlink int main() {
	struct dirent *root_dir;
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

	init_fs(&root_dir);

	free_memory_start = mem_free();

	init_kernel_task(root_dir);

	/* stdout dosyasi denemesi: */
	// FileMode fm;
	// fm.type = FileMode::FT_chrdev;
	// fm.write = 1;
	// r = do_mknod("/stdout", fm, 11);
	// ASSERT(r == 0);
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
	char* argv[1] = { NULL };

	print_info(">> init_task started\n");

	do_execve("/initrd/init", argv);
	PANIC("/initrd/init baslatilamadi");
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
