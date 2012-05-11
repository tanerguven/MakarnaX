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

#include <kernel/kernel.h>
#include <asm/io.h>
#include <asm/x86.h>
#include <string.h>

#include "task.h"
#include "sched.h"

// keyboard.cpp
void keyboard_init();
void keyboard_interrupt();

static void serial_init();
static void cga_putc(int c);
asmlink void console_putc(int c);

static uint16_t COM1 = 0;
static uint16_t COM2 = 0;
static uint16_t COM3 = 0;
static uint16_t COM4 = 0;

static void COM1_putc(int) __attribute__((unused));
static void COM2_putc(int) __attribute__((unused));
static void COM3_putc(int) __attribute__((unused));
static void COM4_putc(int) __attribute__((unused));

static int COM1_getc() __attribute__((unused));
static int COM2_getc() __attribute__((unused));
static int COM3_getc() __attribute__((unused));
static int COM4_getc() __attribute__((unused));

/****************************************************
 * Aygittan bagimsiz console islemleri
 ****************************************************/

struct Console {
	uint8_t buff[512];
	uint32_t rpos;
	uint32_t wpos;

	inline void init() {
		rpos = wpos = 0;
	}
};

static struct spinlock console_lock;
static Console console;
TaskList_t console_input_list;

void init_console() {
	spinlock_init(&console_lock);
	console.init();
	serial_init();
	keyboard_init();
	console_input_list.init();
}

/*
 * aygit interrupt fonksiyonundan cagirilan, girdiyi console input bufferina
 * aktaran fonksiyon
 */
void console_interrupt(int (*proc)()) {
	int c;

	spinlock_acquire(&console_lock);
	while ( (c = (*proc)()) != -1) {
		if (c == 0)
			continue;
		console.buff[console.wpos++] = c;
		if (console.wpos == 512)
			console.wpos = 0;
		wakeup_interruptible(&console_input_list);
	}
	spinlock_release(&console_lock);
}

asmlink int console_getc() {
	int c = 0;

	keyboard_interrupt();

	#ifdef __CONF_use_COM1_io
	if (COM1)
		console_interrupt(COM1_getc);
	#endif

	spinlock_acquire(&console_lock);
	if (console.rpos != console.wpos) {
		c = console.buff[console.rpos++];
		if (console.rpos == 512)
			console.rpos = 0;
	}
	spinlock_release(&console_lock);

	return c;
}


asmlink void console_putc(int c) {
	/* console'a bağlı tüm çıkış aygıtlarına, çıktıyı gönder */
	cga_putc(c);

	#ifdef __CONF_use_COM1_io
	if (COM1)
		COM1_putc(c);
	#endif
}

/****************************************************
 * Console kullanimi icin ust seviye islemler
 ****************************************************/

// asmlink int iscons(int fdnum) {
// 	return 1;
// }

/*******************************************************************
 * 							Serial
 *******************************************************************/

static void find_com_ports() {
	// http://wiki.osdev.org/Memory_Map_(x86)#BIOS_Data_Area_.28BDA.29
	COM1 = *(uint16_t*)0x0400;
	COM2 = *(uint16_t*)0x0402;
	COM3 = *(uint16_t*)0x0404;
	COM4 = *(uint16_t*)0x0406;
}

static void init_serial(uint16_t port) {
	uint32_t eflags = eflags_read();
	cli();
	outb(port + 1, 0x00);
	outb(port + 3, 0x80);
	outb(port + 0, 0x03);
	outb(port + 1, 0x00);
	outb(port + 3, 0x03);
	outb(port + 2, 0xC7);
	outb(port + 4, 0x0B);
	eflags_load(eflags);
}

static void serial_init() {
	find_com_ports();
	init_serial(COM1);
}

#define COM_N_getc(COM) \
static int COM##_getc() {	\
	int r; \
	ASSERT(COM != 0); \
	pushcli(); \
	if ((inb(COM1 + 5) & 1) == 0) /* if not received */ \
		r = -1; \
	else \
		r = inb(COM1); \
	popif();; \
	return r; \
}

COM_N_getc(COM1);
COM_N_getc(COM2);
COM_N_getc(COM3);
COM_N_getc(COM4);

#define COM_N_putc(COM) \
static void COM##_putc(int c) { \
	ASSERT(COM != 0); \
	pushcli(); \
	while ((inb(COM + 5) & 0x20) == 0) \
		/* if transmit not empty, wait */; \
	outb(COM, c); \
	popif(); \
}

COM_N_putc(COM1);
COM_N_putc(COM2);
COM_N_putc(COM3);
COM_N_putc(COM4);

/*******************************************************************
 * 							CGA
 *******************************************************************/

#define CRTPORT 0x3d4

/*
 * 16 kb CGA memory
 * ekran 25 x 80 karakter, her karakter 16 bit (ekranda gorulen karakterler 4000 byte)
 * 16 kb ile 4 sayfa kullanilabilir
 * Burada sadece ekranda goruntulenen 4000 byte bellek kullaniliyor
 */
static uint16_t (*cga_memory)[80] = (uint16_t(*)[80])0xb8000;

#define TAB_SIZE 8
#define CHAR_COLOR 0x0700

static void cga_putc(int c) {
	int cursor_pos;

	pushcli();

	// read cursor position
	outb(CRTPORT, 14);
	cursor_pos = inb(CRTPORT + 1) << 8;
	outb(CRTPORT, 15);
	cursor_pos |= inb(CRTPORT + 1);

	uint8_t cursor_x = cursor_pos % 80;
	uint8_t cursor_y = cursor_pos / 80;

	switch (c & 0xff) {
	case '\b': //backspace
		if (cursor_x | cursor_y) {
			if (cursor_x-- == 0) {
				cursor_x = 0;
				cursor_y--;
			}
			cga_memory[cursor_y][cursor_x] = (' ' & 0xff) | CHAR_COLOR;
		}
		break;
	case '\n':
		cursor_y++;
		cursor_x = 0;
		break;
	case '\t':
		cursor_x += TAB_SIZE - (cursor_x % TAB_SIZE);
		break;
	default:
		cga_memory[cursor_y][cursor_x] = (c & 0xff) | CHAR_COLOR;
		if (++cursor_x == 0) {
			cursor_x = 0;
			cursor_y++;
		}
	}

	if (cursor_y > 24) { // Scroll up
		memmove(&cga_memory[0][0], &cga_memory[1][0], sizeof(cga_memory[0]) * 25);
		cursor_y--;
		memset(&cga_memory[cursor_y][cursor_x], 0, sizeof(cga_memory[0]) - cursor_x);
	}

	cursor_pos = cursor_y * 80 + cursor_x;

	outb(CRTPORT, 14);
	outb(CRTPORT + 1, cursor_pos >> 8);
	outb(CRTPORT, 15);
	outb(CRTPORT + 1, cursor_pos);
	cga_memory[cursor_y][cursor_x] = ' ' | CHAR_COLOR;

	popif();
}
