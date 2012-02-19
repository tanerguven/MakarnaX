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
extern struct Task* task_create(void* program_addr, const char *cmd, int priority);
extern void schedule_init();
extern void run_first_task();
extern void start_kernel_monitor();
extern void picirq_init();
extern void timer_init();
extern void ipc_init();
extern void init_vfs(struct Task *init_task);

extern uint32_t free_memory_start;

void test();

asmlink int main() {
	struct Task* init_task;

	init_console();
	printf("\n");

#ifdef M_DEBUG_1
	printf("DEBUG 1\n");
#endif
#ifdef M_DEBUG_2
	printf("DEBUG 2\n");
#endif

	memory_init();

	init_traps();
	task_init();

	schedule_init();
	ipc_init();

	picirq_init();
	timer_init();

	free_memory_start = mem_free();

	init_task = task_create(&USER_PROGRAM(init), "init", 1);

	init_vfs(init_task);

	printf("teste baslamak icin bir tusa basin\n");
	getchar();
	test();

	run_first_task();

	return 0;
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
};
// FIXME: const
size_t nr_user_programs = sizeof(user_programs)/sizeof(user_programs[0]);

UserProgram *user_program(const char *name) {
	for (int i = 0 ; i < nr_user_programs ; i++) {
		if ( strcmp(user_programs[i].name, name) == 0 )
			return &user_programs[i];
	}
	return NULL;
}

/** init_programs dosyasini okuyup proses olusturur */
void test() {
	char init_programs[1000];
	int size = (int)&_binary_init_programs_size;
	char *lines[10];
	int i_line = 0;

	memcpy(init_programs, &_binary_init_programs_start, size);

	printf("init_programs:\n");

	/* dosyayi satir satir ayir */
	lines[0] = init_programs;
	for (int i = 0 ; i <= size ; i++) {
		if ( (init_programs[i] == '\n') || (i == size) ) {
			init_programs[i] = '\0';
			printf("- %s\n", lines[i_line]);
			i_line++;
			if (i != size)
				lines[i_line] = &init_programs[i+1];
		}
	}

	/* her satir icin bir proses olustur */
	for (int i = 0 ; i < i_line ; i++) {
		char prog[50];
		for (int j = 0 ; ; j++) {
			prog[j] = lines[i][j];
			if (lines[i][j] == ' ') {
				prog[j] = '\0';
				printf("prog: %s\n", prog);
				printf("cmd: %s\n", lines[i]);
				UserProgram *p = user_program(prog);
				ASSERT(p != NULL);
				task_create(p->addr, lines[i], DEFAULT_PRIORITY);
				break;
			}
		}
	}

}
