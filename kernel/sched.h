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

#ifndef SCHED_H_
#define SCHED_H_

#include "task.h"
#include <wmc/list.h>

extern void sleep_interruptible(TaskList_t *list);
extern void wakeup_interruptible(TaskList_t *list);

extern void sleep_uninterruptible(TaskList_t *list);
extern void wakeup_uninterruptible(TaskList_t *list);

extern void schedule();

extern void add_to_runnable_list(Task* t);
extern void remove_from_runnable_list(Task* t);

#endif /* SCHED_H_ */
