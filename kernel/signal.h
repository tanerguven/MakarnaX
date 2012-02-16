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

#ifndef _SIGNAL_H_
#define _SIGNAL_H_

extern void signal_return(struct Trapframe *tf);
extern int send_signal(uint32_t sig, struct Task* t);
extern void check_signals();

define_list(struct SignalState, SignalStack_t);

struct SignalState {
	Page* stack;
	uint32_t esp;
	uint32_t eip;

	uint32_t sleep;
	SignalStack_t::node_t list_node;
} __attribute__((packed));

struct SignalAction {
	uint32_t handler;
};

struct SignalInfo {
	uint32_t sig;
	uint32_t pending;
	SignalAction action[32];
	inline void init() {
		sig = 0;
		pending = 0;
	}
};

#endif /* _SIGNAL_H_ */
