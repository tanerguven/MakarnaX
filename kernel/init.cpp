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
#include <stdio.h>
using namespace std;

#include "test_programs.h"
#include "memory/physical.h"

extern void init_console();
extern int memory_init();
extern void init_traps();
extern void task_init();
extern void schedule_init();
extern void run_first_task();
extern void start_kernel_monitor();
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

asmlink int main() {
	struct DirEntry *root_dir;
	int r;

	init_console();
	printf("\n");

#ifdef M_DEBUG_1
	printf("DEBUG 1\n");
#endif
#ifdef M_DEBUG_2
	printf("DEBUG 2\n");
#endif

	task_init();

	memory_init();

	init_traps();

	schedule_init();
	ipc_init();

	picirq_init();
	timer_init();

	root_dir = init_vfs();

	free_memory_start = mem_free();

	printf("init_kernel_task\n");
	init_kernel_task(root_dir);

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
	printf(">> kernel_task started\n");
	while (1) {
		sti();
		asm("hlt");
		cli();
	}
}

void init_task() {
	int r;
	char* argv[1] = { NULL };

	printf(">> init_task started\n");

	r = do_execve("init", argv);
	ASSERT(r == 0);
}

void __panic(const char *msg, const char* file, int line) {
	asm("cli");
	asm("cld");
	printf("Panic: %s:%d\n", file, line);
	printf("%s\n", msg);
	start_kernel_monitor();
	while(1) {
		asm("hlt");
	}
}

void __panic_assert(const char* file, int line, const char* d) {
	asm("cli");
	asm("cld");
	printf("Panic: %s:%d\n", file, line);
	printf("assertion failed: %s\n", d);
	start_kernel_monitor();
	while(1) {
		asm("hlt");
	}
}


/******************************
 * Test programlari
 ******************************/
UserProgram user_programs[] = {
	{ "yield", &USER_PROGRAM(yield), &USER_PROGRAM_END(yield) },
	{ "hello", &USER_PROGRAM(hello), &USER_PROGRAM_END(hello) },
	{ "divide_error", &USER_PROGRAM(divide_error), &USER_PROGRAM_END(divide_error) },
	{ "forktest", &USER_PROGRAM(forktest), &USER_PROGRAM_END(forktest) },
	{ "dongu", &USER_PROGRAM(dongu), &USER_PROGRAM_END(dongu) },
	{ "sys_dongu", &USER_PROGRAM(sys_dongu), &USER_PROGRAM_END(sys_dongu) },
	{ "signaltest", &USER_PROGRAM(signaltest), &USER_PROGRAM_END(signaltest) },
	{ "ipctest", &USER_PROGRAM(ipctest), &USER_PROGRAM_END(ipctest) },
	{ "processmemtest", &USER_PROGRAM(processmemtest),
	  &USER_PROGRAM_END(processmemtest) },
	{ "kill", &USER_PROGRAM(kill), &USER_PROGRAM_END(kill) },
	{ "fs", &USER_PROGRAM(fs), &USER_PROGRAM_END(fs) },
	{ "init", &USER_PROGRAM(init), &USER_PROGRAM_END(init) },
};
// FIXME: const
size_t nr_user_programs = sizeof(user_programs)/sizeof(user_programs[0]);

UserProgram *user_program(const char *name) {
	for (unsigned int i = 0 ; i < nr_user_programs ; i++) {
		if ( strcmp(user_programs[i].name, name) == 0 )
			return &user_programs[i];
	}
	return NULL;
}
