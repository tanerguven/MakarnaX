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

#ifndef TEST_PROGRAMS_H_
#define TEST_PROGRAMS_H_

#include <types.h>

struct TestProgram {
	const char *name;
	void *addr;
	void *end;
};

extern const TestProgram test_programs[];
extern const size_t nr_test_programs;
extern const TestProgram *test_program(const char *name);

#define TEST_PROGRAM(x) _binary_user_programs_test_##x##_bin_start
#define TEST_PROGRAM_END(x) _binary_user_programs_test_##x##_bin_end
#define TEST_PROGRAM_SIZE(x) _binary_user_programs_test_##x##_bin_size
#define DEFINE_TEST_PROGRAM(x)					\
	extern void* TEST_PROGRAM(x);				\
	extern void* TEST_PROGRAM_END(x);			\
	extern void* TEST_PROGRAM_SIZE(x);

/* test programlari */

DEFINE_TEST_PROGRAM(divide_error);
DEFINE_TEST_PROGRAM(hello);
DEFINE_TEST_PROGRAM(yield);
DEFINE_TEST_PROGRAM(forktest);
DEFINE_TEST_PROGRAM(dongu);
DEFINE_TEST_PROGRAM(sys_dongu);
DEFINE_TEST_PROGRAM(signaltest);
DEFINE_TEST_PROGRAM(init);
DEFINE_TEST_PROGRAM(ipctest);
DEFINE_TEST_PROGRAM(processmemtest);
DEFINE_TEST_PROGRAM(kill);
DEFINE_TEST_PROGRAM(fs);
DEFINE_TEST_PROGRAM(hello_newlib);

/* init_programs dosyasi */
extern void* _binary_init_programs_start;
extern void* _binary_init_programs_size;
extern void* _binary_init_programs_end;

#define DEFAULT_PRIORITY 3

#endif /* TEST_PROGRAMS_H_ */
