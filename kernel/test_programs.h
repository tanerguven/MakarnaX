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

#ifndef TEST_PROGRAMS_H_
#define TEST_PROGRAMS_H_

#include <types.h>

#define USER_PROGRAM(x) _binary_user_programs_##x##_bin_start
extern void* USER_PROGRAM(divide_error);
extern void* USER_PROGRAM(hello);
extern void* USER_PROGRAM(yield);
extern void* USER_PROGRAM(forktest);
extern void* USER_PROGRAM(dongu);
extern void* USER_PROGRAM(sys_dongu);
extern void* USER_PROGRAM(signaltest);
extern void* USER_PROGRAM(init);
extern void* USER_PROGRAM(ipctest);
extern void* USER_PROGRAM(processmemtest);
extern void* USER_PROGRAM(kill);
extern void* USER_PROGRAM(fs);

extern int load_program(uint32_t addres);


#define DEFAULT_PRIORITY 3

#define test_program(program,param) do { \
	task_create(&USER_PROGRAM(program), #program" "#param, DEFAULT_PRIORITY); \
	} while (0)

/*
 * test programlari
 * MAKARNAX_TEST_dongu, "make run-%" komutunda dongu olarak kullanilir
 */
#define MAKARNAX_TEST_dongu 1
#define MAKARNAX_TEST_sleep 2
#define MAKARNAX_TEST_hello 3
#define MAKARNAX_TEST_dongu2 4
#define MAKARNAX_TEST_fork 5
#define MAKARNAX_TEST_signal 6
#define MAKARNAX_TEST_keyboard 7
#define MAKARNAX_TEST_fork2 8
#define MAKARNAX_TEST_signal4 9
#define MAKARNAX_TEST_signal5 10
#define MAKARNAX_TEST_signal6 11
#define MAKARNAX_TEST_ipc_shmtest 12
#define MAKARNAX_TEST_ipc_shm 13
#define MAKARNAX_TEST_ipc_shm2 14
#define MAKARNAX_TEST_ipc_shm3 15
#define MAKARNAX_TEST_ipc_sem 16
#define MAKARNAX_TEST_ipc_shmfork 17
#define MAKARNAX_TEST_stack 18
#define MAKARNAX_TEST_brktest 19
#define MAKARNAX_TEST_signal7 20
#define MAKARNAX_TEST_signal2 21
#define MAKARNAX_TEST_fs_openfile 22
#define MAKARNAX_TEST_fs_readfile 23

#endif /* TEST_PROGRAMS_H_ */
