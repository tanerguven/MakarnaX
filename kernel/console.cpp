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
#include <asm/io.h>
#include <string.h>
#include <asm/x86.h>

#include "task.h"
#include "sched.h"

// keyboard.cpp
void keyboard_init();
void keyboard_interrupt();
//

//
static void serial_init();
static void serial_putc(int c);
static void serial_interrupt();
static void cga_putc(int c);
void console_putc(int c);
//

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

static Console console;

TaskList_t console_input_list;

void init_console() {
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
	ASSERT(!(eflags_read() & FL_IF));

	while ( (c = (*proc)()) != -1) {
		if (c == 0)
			continue;
		console.buff[console.wpos++] = c;
		if (console.wpos == 512)
			console.wpos = 0;
		// console_putc(c);
		// wakeup_interruptible_2(&console_input_list, c);
		wakeup_interruptible(&console_input_list);
	}
}

int console_getc() {
	// ASSERT(!(eflags_read() & FL_IF));
	uint32_t eflags = eflags_read();
	cli();

	keyboard_interrupt();
	// FIXME: bir makroya gore etkinlestir
	#if 0
	serial_interrupt();
	#endif

	if (console.rpos != console.wpos) {
		int c = console.buff[console.rpos++];
		if (console.rpos == 512)
			console.rpos = 0;
		eflags_load(eflags);
		return c;
	}

	eflags_load(eflags);
	return 0;
}


void console_putc(int c) {
	/* console'a bağlı tüm çıkış aygıtlarına, çıktıyı gönder */
	cga_putc(c);
	serial_putc(c);
}

/****************************************************
 * Console kullanimi icin ust seviye islemler
 ****************************************************/

/** bir karakter girdi okur. girdi yoksa gelene kadar bekler */
asmlink int getchar() {
	int c;

	uint32_t eflags = eflags_read();
	sti();
	while ((c = console_getc()) == 0) {
		/* karakter yoksa kesme gelene kadar bekle */
		asm("hlt");
	}
	eflags_load(eflags);
	return c;
}

asmlink void putchar(int c) {
	console_putc(c);
}

asmlink int iscons(int fdnum) {
	return 1;
}

/*******************************************************************
 * 							Serial
 *******************************************************************/

static int serial_port = 0x3f8;

static void serial_init() {
	uint32_t eflags = eflags_read();
	cli();
	outb(serial_port + 1, 0x00);
	outb(serial_port + 3, 0x80);
	outb(serial_port + 0, 0x03);
	outb(serial_port + 1, 0x00);
	outb(serial_port + 3, 0x03);
	outb(serial_port + 2, 0xC7);
	outb(serial_port + 4, 0x0B);
	eflags_load(eflags);
}
static int serial_getc() {
	int r;

	uint32_t eflags = eflags_read();
	cli();
	if ((inb(serial_port + 5) & 1) == 0) /* if not received */
		r = -1;
	else
		r = inb(serial_port);
	eflags_load(eflags);
	return r;
}

static void serial_putc(int c) {
	uint32_t eflags = eflags_read();
	cli();

	while ((inb(serial_port + 5) & 0x20) == 0)
		/* if transmit not empty, wait */;
	outb(serial_port, c);
	eflags_load(eflags);
}

static void serial_interrupt(void) {
	console_interrupt(serial_getc);
}


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

	uint32_t eflags = eflags_read();
	cli();

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
				cursor_x == 0;
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

	eflags_load(eflags);
}
