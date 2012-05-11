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

#include <kernel/syscall.h>

#include "../trap.h"
#include "../task.h"
#include "ipc.h"

#define SEMGET 1
#define SEMCTL 2
#define SEMOP 3
#define SEMDT 4
#define SEMWAIT 5
#define SEMPOST 6
#define SEMGETVAL 7
#define SHMGET 10
#define SHMAT 11
#define SHMDT 12
#define SHMCTL 12

extern void shm_init();
extern int shm_fork(struct Task* child);
extern void shm_task_free(Task* t);
extern uint32_t mem_shm();
extern void sem_init();

static void no_ipc();

extern void sys_semget();
extern void sys_semctl();
extern void sys_semop();
extern void sys_semwait();
extern void sys_sempost();
extern void sys_semgetvalue();

extern void sys_shmget();
extern void sys_shmat();
extern void sys_shmdt();
extern void sys_shmctl();

static void (*ipc_func[])() = {
	no_ipc,
	sys_semget, sys_semctl, sys_semop, no_ipc, sys_semwait,
	sys_sempost, sys_semgetvalue, no_ipc, no_ipc, sys_shmget, /* 10 */
	sys_shmat, sys_shmdt, sys_shmctl,
};

static void no_ipc() {
	uint32_t ipc_no = get_param1(task_curr->registers());
	print_warning("ipc no: %d\n", ipc_no);
	PANIC("no_ipc");
}


SYSCALL_DEFINE1(ipc, unsigned int, ipc_no) {
	if (ipc_no < sizeof(ipc_func)+1)
		ipc_func[ipc_no]();
	else
		no_ipc();
}
SYSCALL_END(ipc)


void ipc_init() {
	shm_init();
	sem_init();
}

int ipc_fork(struct Task* child) {
	int r;

	r = shm_fork(child);
	if (r < 0)
		return r;
	return 0;
}

extern void ipc_task_free(Task *t) {
	shm_task_free(t);
}


uint32_t mem_ipc() {
	return mem_shm();
}
