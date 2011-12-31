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

/*
 * hata ayiklamak icin kullanilan PANIC ve ASSERT fonksiyonlari/makrolari
 */

extern void __panic(const char *msg, const char* file, int line);
extern void __panic_assert(const char* file, int line, const char* d);

#define PANIC(msg) __panic(msg, __FILE__, __LINE__);
#define ASSERT(b) ((b) ? (void)0 : __panic_assert(__FILE__, __LINE__, #b))


#ifdef M_DEBUG_1
#define ASSERT_D1(b) ASSERT(b)
#else
#define ASSERT_D1(b) /* */
#endif

#ifdef M_DEBUG_2
#define ASSERT_D2(b) ASSERT(b)
#else
#define ASSERT_D2(b) /* */
#endif
