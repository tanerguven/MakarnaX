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

#include <types.h>
#include <asm/x86.h>
#include <stdio.h>
using namespace std;

#include "panic.h"

#include "test_programs.h"
#include "memory/physical.h"

extern void init_console();
extern int memory_init();
extern void init_traps();
extern void task_init();
extern void task_create(void* program_addr, const char *cmd, int priority);
extern void schedule_init();
extern void schedule();
extern void start_kernel_monitor();
extern void picirq_init();
extern void timer_init();
extern void ipc_init();

extern uint32_t free_memory_start;

void test();

asmlink int main() {

	init_console();

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

	task_create(&USER_PROGRAM(init), "init", 1);

#ifdef TEST
	printf("teste baslamak icin bir tusa basin\n");
	getchar();
	test();
	schedule();
#endif


	PANIC("init processi yok");

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


void test() {
#if TEST == MAKARNAX_TEST_dongu
	/* dongu testi */
	test_program(dongu, 0 30000);
	test_program(dongu, 0 30000);
	test_program(dongu, 0 30000);
#endif

#if TEST == MAKARNAX_TEST_sleep
	/* dongu & sleep testi */
	test_program(dongu, 1 100);
	test_program(dongu, 1 100);
#endif

#if TEST == MAKARNAX_TEST_hello
	test_program(hello,);
#endif

#if TEST == MAKARNAX_TEST_dongu2
	/* sonsuz dongu 2 testi */
	test_program(sys_dongu,);
	test_program(dongu,);
	test_program(dongu,);
#endif

#if TEST == MAKARNAX_TEST_fork
	/* fork testi */
	for (int i = 0 ; i < 120 ; i++)
		test_program(forktest,test1);
#endif

#if TEST == MAKARNAX_TEST_signal
	/* signal testi */
	test_program(signaltest,test1);
	test_program(signaltest,test2);
#endif

#if TEST == MAKARNAX_TEST_keyboard
	/* klavye kesmesi testi */
	test_program(signaltest,test3);
	test_program(hello,);
#endif

#if TEST == MAKARNAX_TEST_fork2
	/* wait testi */
	test_program(forktest,test2);
#endif

#if TEST == MAKARNAX_TEST_signal4
	/* pause testi */
	test_program(signaltest,test4);
#endif

#if TEST == MAKARNAX_TEST_signal5
	/* sleep - signal testi */
	test_program(signaltest,test5);
#endif

#if TEST == MAKARNAX_TEST_signal6
	/* pause - 3 signal testi */
	test_program(signaltest,test6);
#endif

#if TEST == MAKARNAX_TEST_ipc_shmtest
	/* ipc testi */
	test_program(ipctest,shmtest);
#endif

#if TEST == MAKARNAX_TEST_ipc_shm
	/* shared memory server - client */
	test_program(ipctest,shmserver);
	test_program(ipctest,shmclient);
#endif

#if TEST == MAKARNAX_TEST_ipc_shm2
	/* ipc shared memory testi */
	test_program(ipctest,shmserver2);
	test_program(ipctest,shmclient2);
#endif

#if TEST == MAKARNAX_TEST_ipc_shm3
	/* ipc semaphore - shared memory testi */
	test_program(ipctest,shmserver3);
	test_program(ipctest,shmclient3);
#endif

#if TEST == MAKARNAX_TEST_ipc_sem
	/* ipc semaphore testi */
	test_program(ipctest,semtest);
#endif

#if TEST == MAKARNAX_TEST_ipc_shmfork
	/* shared memory fork */
	test_program(ipctest,shmfork);
#endif

#if TEST == MAKARNAX_TEST_stack
	/* stacktest */
	test_program(processmemtest,stacktest);
	test_program(processmemtest,stacklimit);
#endif

#if TEST == MAKARNAX_TEST_brktest
	/* stacktest */
	test_program(processmemtest,brktest);
#endif

}
