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

/*
 * FIXME: Dosyanin bir kismi xv6'dan alindi. Lisans nasil kullanilacak ?
 */

/*
 * Copyright (c) 2006-2009 Frans Kaashoek, Robert Morris, Russ Cox,
 *                        Massachusetts Institute of Technology
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE
 * LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
 * OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
 * WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#include <kernel/kernel.h>
#include <asm/io.h>
#include <asm/x86.h>

#include <string.h>

// console.cpp
extern void console_interrupt(int (*proc)());

// kernel_monitor.cpp
extern void start_kernel_monitor();

/** kbd controller status port(I) */
#define	KBSTATP 0x64
/** kbd data in buffer */
#define	KBS_DIB  0x01
/** kbd data port(I) */
#define	KBDATAP	0x60

#define NO		0

#define SHIFT		(1<<0)
#define CTL		(1<<1)
#define ALT		(1<<2)

#define CAPSLOCK	(1<<3)
#define NUMLOCK		(1<<4)
#define SCROLLLOCK	(1<<5)

#define E0ESC		(1<<6)

// Special keycodes
#define KEY_HOME	0xE0
#define KEY_END		0xE1
#define KEY_UP		0xE2
#define KEY_DN		0xE3
#define KEY_LF		0xE4
#define KEY_RT		0xE5
#define KEY_PGUP	0xE6
#define KEY_PGDN	0xE7
#define KEY_INS		0xE8
#define KEY_DEL		0xE9

static uint8_t shiftcode[256];

static uint8_t togglecode[256];

static uint8_t normalmap[256] =
{
	NO,   0x1B, '1',  '2',  '3',  '4',  '5',  '6',	// 0x00
	'7',  '8',  '9',  '0',  '-',  '=',  '\b', '\t',
	'q',  'w',  'e',  'r',  't',  'y',  'u',  'i',	// 0x10
	'o',  'p',  '[',  ']',  '\n', NO,   'a',  's',
	'd',  'f',  'g',  'h',  'j',  'k',  'l',  ';',	// 0x20
	'\'', '`',  NO,   '\\', 'z',  'x',  'c',  'v',
	'b',  'n',  'm',  ',',  '.',  '/',  NO,   '*',	// 0x30
	NO,   ' ',  NO,   NO,   NO,   NO,   NO,   NO,
	NO,   NO,   NO,   NO,   NO,   NO,   NO,   '7',	// 0x40
	'8',  '9',  '-',  '4',  '5',  '6',  '+',  '1',
	'2',  '3',  '0',  '.',  NO,   NO,   NO,   NO,	// 0x50
	NO,      NO,      NO,      NO,      NO,      NO,      NO,      NO,
	NO,      NO,      NO,      NO,      NO,      NO,      NO,      NO, // 0x60
	NO,      NO,      NO,      NO,      NO,      NO,      NO,      NO,
	NO,      NO,      NO,      NO,      NO,      NO,      NO,      NO, // 0x70
	NO,      NO,      NO,      NO,      NO,      NO,      NO,      NO,
	NO,      NO,      NO,      NO,      NO,      NO,      NO,      NO, // 0x80
	NO,      NO,      NO,      NO,      NO,      NO,      NO,      NO,
	NO,      NO,      NO,      NO,      NO,      NO,      NO,      NO, // 0x90
	NO,      NO,      NO,      NO,      '\n',    NO,      NO,      NO,
	NO,      NO,      NO,      NO,      NO,      NO,      NO,      NO, // 0xA0
	NO,      NO,      NO,      NO,      NO,      NO,      NO,      NO,
	NO,      NO,      NO,      NO,      NO,      '/',     NO,      NO, // 0xB0
	NO,      NO,      NO,      NO,      NO,      NO,      NO,      NO,
	NO,      NO,      NO,      NO,      NO,      NO,      NO,      KEY_HOME, // 0xC0
	KEY_UP,  KEY_PGUP,NO,      KEY_LF,  NO,      KEY_RT,  NO,      KEY_END,
	KEY_DN,  KEY_PGDN,KEY_INS, KEY_DEL, NO,      NO,      NO,      NO, // 0xD0
	NO,      NO,      NO,      NO,      NO,      NO,      NO,      NO,
	NO,      NO,      NO,      NO,      NO,      NO,      NO,      NO, // 0xE0
	NO,      NO,      NO,      NO,      NO,      NO,      NO,      NO,
	NO,      NO,      NO,      NO,      NO,      NO,      NO,      NO, // 0xF0
	NO,      NO,      NO,      NO,      NO,      NO,      NO,      NO,
};

static uint8_t shiftmap[256] =
{
	NO,   033,  '!',  '@',  '#',  '$',  '%',  '^',	// 0x00
	'&',  '*',  '(',  ')',  '_',  '+',  '\b', '\t',
	'Q',  'W',  'E',  'R',  'T',  'Y',  'U',  'I',	// 0x10
	'O',  'P',  '{',  '}',  '\n', NO,   'A',  'S',
	'D',  'F',  'G',  'H',  'J',  'K',  'L',  ':',	// 0x20
	'"',  '~',  NO,   '|',  'Z',  'X',  'C',  'V',
	'B',  'N',  'M',  '<',  '>',  '?',  NO,   '*',	// 0x30
	NO,   ' ',  NO,   NO,   NO,   NO,   NO,   NO,
	NO,   NO,   NO,   NO,   NO,   NO,   NO,   '7',	// 0x40
	'8',  '9',  '-',  '4',  '5',  '6',  '+',  '1',
	'2',  '3',  '0',  '.',  NO,   NO,   NO,   NO,	// 0x50
	NO,      NO,      NO,      NO,      NO,      NO,      NO,      NO,
	NO,      NO,      NO,      NO,      NO,      NO,      NO,      NO, // 0x60
	NO,      NO,      NO,      NO,      NO,      NO,      NO,      NO,
	NO,      NO,      NO,      NO,      NO,      NO,      NO,      NO, // 0x70
	NO,      NO,      NO,      NO,      NO,      NO,      NO,      NO,
	NO,      NO,      NO,      NO,      NO,      NO,      NO,      NO, // 0x80
	NO,      NO,      NO,      NO,      NO,      NO,      NO,      NO,
	NO,      NO,      NO,      NO,      NO,      NO,      NO,      NO, // 0x90
	NO,      NO,      NO,      NO,      '\n',    NO,      NO,      NO,
	NO,      NO,      NO,      NO,      NO,      NO,      NO,      NO, // 0xA0
	NO,      NO,      NO,      NO,      NO,      NO,      NO,      NO,
	NO,      NO,      NO,      NO,      NO,      '/',     NO,      NO, // 0xB0
	NO,      NO,      NO,      NO,      NO,      NO,      NO,      NO,
	NO,      NO,      NO,      NO,      NO,      NO,      NO,      KEY_HOME, // 0xC0
	KEY_UP,  KEY_PGUP,NO,      KEY_LF,  NO,      KEY_RT,  NO,      KEY_END,
	KEY_DN,  KEY_PGDN,KEY_INS, KEY_DEL, NO,      NO,      NO,      NO, // 0xD0
	NO,      NO,      NO,      NO,      NO,      NO,      NO,      NO,
	NO,      NO,      NO,      NO,      NO,      NO,      NO,      NO, // 0xE0
	NO,      NO,      NO,      NO,      NO,      NO,      NO,      NO,
	NO,      NO,      NO,      NO,      NO,      NO,      NO,      NO, // 0xF0
	NO,      NO,      NO,      NO,      NO,      NO,      NO,      NO,
};

#define C(x) (uint8_t)(x - '@')

static uint8_t ctlmap[256] =
{
	NO,      NO,      NO,      NO,      NO,      NO,      NO,      NO, // 0x00
	NO,      NO,      NO,      NO,      NO,      NO,      NO,      NO,
	C('Q'),  C('W'),  C('E'),  C('R'),  C('T'),  C('Y'),  C('U'),  C('I'), // 0x10
	C('O'),  C('P'),  NO,      NO,      '\r',    NO,      C('A'),  C('S'),
	C('D'),  C('F'),  C('G'),  C('H'),  C('J'),  C('K'),  C('L'),  NO, // 0x20
	NO,      NO,      NO,      C('\\'), C('Z'),  C('X'),  C('C'),  C('V'),
	C('B'),  C('N'),  C('M'),  NO,      NO,      C('/'),  NO,      NO, // 0x30
	NO,      NO,      NO,      NO,      NO,      NO,      NO,      NO,
	NO,      NO,      NO,      NO,      NO,      NO,      NO,      NO, // 0x40
	NO,      NO,      NO,      NO,      NO,      NO,      NO,      NO,
	NO,      NO,      NO,      NO,      NO,      NO,      NO,      NO, // 0x50
	NO,      NO,      NO,      NO,      NO,      NO,      NO,      NO,
	NO,      NO,      NO,      NO,      NO,      NO,      NO,      NO, // 0x60
	NO,      NO,      NO,      NO,      NO,      NO,      NO,      NO,
	NO,      NO,      NO,      NO,      NO,      NO,      NO,      NO, // 0x70
	NO,      NO,      NO,      NO,      NO,      NO,      NO,      NO,
	NO,      NO,      NO,      NO,      NO,      NO,      NO,      NO, // 0x80
	NO,      NO,      NO,      NO,      NO,      NO,      NO,      NO,
	NO,      NO,      NO,      NO,      NO,      NO,      NO,      KEY_HOME, // 0x90
	NO,      NO,      NO,      NO,      NO,      NO,      NO,      NO,
	NO,      NO,      NO,      NO,      NO,      NO,      NO,      NO, // 0xA0
	NO,      NO,      NO,      NO,      NO,      NO,      NO,      NO,
	NO,      NO,      NO,      NO,      NO,      C('/'),  NO,      NO, // 0xB0
	NO,      NO,      NO,      NO,      NO,      NO,      NO,      NO,
	NO,      NO,      NO,      NO,      NO,      NO,      NO,      NO, // 0xC0
	KEY_UP,  KEY_PGUP,NO,      KEY_LF,  NO,      KEY_RT,  NO,      KEY_END,
	KEY_DN,  KEY_PGDN,KEY_INS, KEY_DEL, NO,      NO,      NO,      NO, // 0xD0
	NO,      NO,      NO,      NO,      NO,      NO,      NO,      NO,
	NO,      NO,      NO,      NO,      NO,      NO,      NO,      NO, // 0xE0
	NO,      NO,      NO,      NO,      NO,      NO,      NO,      NO,
	NO,      NO,      NO,      NO,      NO,      NO,      NO,      NO, // 0xF0
	NO,      NO,      NO,      NO,      NO,      NO,      NO,      NO,
};

int keyboard_getc(void) {
	static uint32_t shift;
	static uint8_t *charcode[4] = {
		normalmap, shiftmap, ctlmap, ctlmap
	};
	uint32_t st, data, c;

	ASSERT_int_disable();

	st = inb(KBSTATP);
	if((st & KBS_DIB) == 0)
		return -1;
	data = inb(KBDATAP);

	if(data == 0xE0){
		shift |= E0ESC;
		return 0;
	} else if(data & 0x80){
		// Key released
		data = (shift & E0ESC ? data : data & 0x7F);
		shift &= ~(shiftcode[data] | E0ESC);
		return 0;
	} else if(shift & E0ESC){
		// Last character was an E0 escape; or with 0x80
		data |= 0x80;
		shift &= ~E0ESC;
	}

	shift |= shiftcode[data];
	shift ^= togglecode[data];
	c = charcode[shift & (CTL | SHIFT)][data];
	if(shift & CAPSLOCK){
		if('a' <= c && c <= 'z')
			c += 'A' - 'a';
		else if('A' <= c && c <= 'Z')
			c += 'a' - 'A';
	}

	if (!(~shift & (CTL | ALT)) && c == KEY_DEL) {
		/* debug modu */
		print_info(">> debug mode: \n");
		start_kernel_monitor();
	}

	return c;
}


void keyboard_interrupt() {
	console_interrupt(keyboard_getc);
}

void keyboard_init() {
	keyboard_interrupt();

	memset(shiftcode, 0, sizeof(shiftcode));
	shiftcode[0x1D] = CTL;
	shiftcode[0x2A] = SHIFT;
	shiftcode[0x36] = SHIFT;
	shiftcode[0x38] = ALT;
	shiftcode[0x9D] = CTL;
	shiftcode[0xB8] = ALT;

	memset(togglecode, 0, sizeof(togglecode));
	togglecode[0x3A] = CAPSLOCK;
	togglecode[0x45] = NUMLOCK;
	togglecode[0x46] = SCROLLLOCK;
}

asmlink void do_irq_keyboard(struct Trapframe* tf) {
	console_interrupt(keyboard_getc);
}
