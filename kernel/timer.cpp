#include <types.h>
#include <asm/io.h>

#include  <kernel/trap.h>

extern void pic_enable(int irq);


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

/* 8253 Timer #1 */
#define IO_TIMER1 0x040

#define TIMER_FREQ 1193182
#define TIMER_DIV(x) ((TIMER_FREQ+(x)/2)/(x))

/* timer mode port */
#define TIMER_MODE (IO_TIMER1 + 3)
/* select counter 0 */
#define TIMER_SEL0 0x00
/* mode 2, rate generator */
#define TIMER_RATEGEN 0x04
/* r/w counter 16 bits, LSB first */
#define TIMER_16BIT 0x30

uint32_t jiffies;

void timer_init() {
	jiffies = 0;
	outb(TIMER_MODE, TIMER_SEL0 | TIMER_RATEGEN | TIMER_16BIT);
	outb(IO_TIMER1, TIMER_DIV(100) % 256);
	outb(IO_TIMER1, TIMER_DIV(100) / 256);
	pic_enable(IRQ_TIMER);

	//FIXME: gecici olarak burda
	pic_enable(IRQ_KBD);
}
