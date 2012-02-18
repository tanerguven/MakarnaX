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

struct UserProgram {
	const char *name;
	void *addr;
	void *end;
};

extern UserProgram user_programs[];
extern size_t nr_user_programs;
UserProgram *user_program(const char *name);

#define USER_PROGRAM(x) _binary_user_programs_##x##_bin_start
#define USER_PROGRAM_END(x) _binary_user_programs_##x##_bin_end
#define USER_PROGRAM_SIZE(x) _binary_user_programs_##x##_bin_size
#define DEFINE_USER_PROGRAM(x)					\
	extern void* USER_PROGRAM(x);				\
	extern void* USER_PROGRAM_END(x);			\
	extern void* USER_PROGRAM_SIZE(x);

/* test programlari */

DEFINE_USER_PROGRAM(divide_error);
DEFINE_USER_PROGRAM(hello);
DEFINE_USER_PROGRAM(yield);
DEFINE_USER_PROGRAM(forktest);
DEFINE_USER_PROGRAM(dongu);
DEFINE_USER_PROGRAM(sys_dongu);
DEFINE_USER_PROGRAM(signaltest);
DEFINE_USER_PROGRAM(init);
DEFINE_USER_PROGRAM(ipctest);
DEFINE_USER_PROGRAM(processmemtest);
DEFINE_USER_PROGRAM(kill);
DEFINE_USER_PROGRAM(fs);

/* init_programs dosyasi */
extern void* _binary_init_programs_start;
extern void* _binary_init_programs_size;
extern void* _binary_init_programs_end;

#define DEFAULT_PRIORITY 3

#endif /* TEST_PROGRAMS_H_ */
