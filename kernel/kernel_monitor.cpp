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
 * FIXME: fikir, jos kernel/monitor.c
 */

#include <asm/x86.h>
#include <types.h>
#include <string.h>
#include <stdio.h>

#include <genel_fonksiyonlar.h>

#include "multiboot.h"
#include <genel_fonksiyonlar.h>
#include "memory/physical.h"

#include "test_programs.h"

#include <wmc/list.h>
#include "task.h"
#include "sched.h"
#include <asm/x86.h>

//
extern void task_create(void* program_addr, const char *cmd, int priority);
extern void schedule();
extern size_t kmalloc_size(size_t size);
//

uint32_t free_memory_start;

bool kernel_monitor_running = false;

static const char* task_state_name[] = {
	"running",
	"interruptible",
	"uninterruptible",
	"zombie",
	"stopped",
};

static int command_help(int argc, char** argv);
static int command_backtrace(int argc, char **argv);
static int command_kernelinfo(int argc, char** argv);
static int command_sysinfo(int argc, char **argv);
static int command_info(int argc, char **argv);
static int command_create(int argc, char **argv);
static int command_continue(int argc, char **argv);
static int command_tasks(int argc, char **argv);
static int command_taskinfo(int argc, char **argv);
static int command_memdebug(int argc, char **argv);

struct Command {
	const char *name;
	const char *shortname;
	const char *description;
	int (*func)(int argc, char** argv);
};

Command commands[] = {
	{ "help", "h", "", command_help },
	{ "kernelinfo", "ki", "kernel info", command_kernelinfo },
	{ "backtrace", "bt", "", command_backtrace },
	{ "sysinfo", "si", "", command_sysinfo },
	{ "info", "i", "", command_info },
	{ "create", "cr", "create process", command_create },
	{ "tasks", "ts", "", command_tasks },
	{ "task", "t", "task info", command_taskinfo },
	{ "continue", "c", "continue", command_continue },
	{ "memdebug", "md", "memory debug", command_memdebug },
};

uint32_t nr_commands = sizeof(commands) / sizeof(commands[0]);

void parse_cmd(char *cmd, int *argc, char *argv[10]) {
	*argc = 0;
	argv[0] = cmd;

	/*
	 * 		cmd = " asd fff\t ggg"
	 * şeklindeki stringi aşağıdaki biçime dönüştürür
	 * 		cmd = "\0asd\0fff\0\0ggg"
	 *
	 * arg'lar, cmd'nin belirli bir karakterini referans eder
	 * 		argv[0] = "asd\0fff\0\0ggg"	-> &cmd[1]
	 * 		argv[1] = "fff\0\0ggg" 		-> &cmd[5]
	 * 		argv[2] = "ggg" 			-> &cmd[10]
	 * 		argc = 3
	 */
	for ( ; *cmd != '\0' ; cmd++) {
		switch (*cmd) {
			case ' ':
			case '\t':
			case '\n':
				*cmd = '\0'; /* boşluk karakterlerini null olarak işaretle */
				if ( *(cmd-1) != '\0' && (*argc) < 10) //10 -> max argc
					(*argc)++;
				argv[*argc] = cmd+1;
				break;
		}
	}

	if (argv[*argc][0] != '\0')
		(*argc)++;
	argv[*argc] = 0;
}

int runcmd(char *cmd) {
	int argc = 0;
	char *argv[10];

	parse_cmd(cmd, &argc, argv);

	/* komutu bul ve çalıştır */
	for (uint32_t i = 0 ; i < nr_commands ; i++) {
		if ( strcmp(commands[i].shortname, argv[0]) == 0 ||
			 strcmp(commands[i].name, argv[0]) == 0)
			return commands[i].func(argc, argv);
	}
	printf("%s: command not found\n", argv[0]);
	return 0;
}


void start_kernel_monitor() {
	uint32_t eflags = eflags_read();
	cli();
	kernel_monitor_running = true;
	while (kernel_monitor_running) {
		ASSERT(!(eflags_read() & FL_IF));
		printf("\n");
		char *buf = readline("$:");
		if (buf) {
			runcmd(buf);
		}
	}
	eflags_load(eflags);
}

static int command_help(int argc, char** argv) {
	printf("command list:\n");
	for (uint32_t i = 0 ; i < nr_commands ; i++) {
		printf("%s (%s) - %s\n", commands[i].name, commands[i].shortname,
			   commands[i].description);
	}
	return 0;
}

static int command_kernelinfo(int argc, char** argv) {
	uint32_t kernelSize = ((uint32_t)&__kernel_end - (uint32_t)&__text_start) >> 10;
	printf("kernel size: %d kb\n", kernelSize);
	printf("text\t0x%08x\n", &__text_start);
	printf("rodata\t0x%08x\n", &__rodata_start);
	printf("data\t0x%08x\n", &__data_start);
	printf("bss\t0x%08x\n", &__bss_start);
	printf("end\t0x%08x\n", &__kernel_end);
	return 0;
}

static int command_backtrace(int argc, char **argv) {
	/*
	 * ----------------------------
	 * 			eip
	 * 0xfff8:	ebp = 0x0
	 * 0xfff4	yerel değişkenler ...
	 * ----------------------------
	 * 0xfff0	arg4
	 * 0xffec	arg3
	 * 0xffe8	arg2
	 * 0xffe4	arg1
	 * 0xffe0	arg0		-> fonksiyonun parametreleri
	 * 0xffdc	eip			-> fonksiyonun çağırıldığı kodun yerinin adresi
	 * 0xffd8	ebp = 0xfff8 -> fonksiyonu çağıran fonksiyonun ebp'sinin stackteki adresi
	 * 			yerel değişkenler ...
	 * ----------------------------
	 * 			arg4
	 * 			...
	 * 			arg0
	 * 0xffbc	eip
	 * 0xffb8	ebp = 0xffd8
	 * 			...
	 * 			yerel değişkenler
	 * 			...
	 * 0xffa0
	 * ----------------------------
	 */

	/*
	 * çalışma sırasındaki registerlar
	 * esp = 0xffa0
	 * ebp = 0xffb8
	 */

	/*
	 * - yeni bir fonksiyon çağırılırken stack'e sırasıyla
	 * arg'lar, eip ve ebp push'lanıyor
	 * - ebp registerına esp aktarılıyor
	 * - esp registerı, fonksiyonun kullanacağı stack alanı kadar arttırılıyor
	 * - fonksiyonun yerel değişkenleri, ebp ve esp registerlarının gösterdiği adresler arasında
	 * saklanıyor
	 */

	printf("ebp: %08x\n", ebp_read());
	printf("ebp: %08x\n", ebp_read());

	for (uint32_t* ebp = (uint32_t*)ebp_read() ;
		 (va2kaddr(MMAP_KERNEL_STACK_TOP) - (uint32_t)ebp) < 0x1000 ;
		 ebp = (uint32_t*)*ebp)
	{
		uint32_t eip = *(ebp+1);

		unsigned int args[5];
		for (int i = 0; i < 5; i++)
			args[i] = *(ebp + 2 + i);

		printf("    %08x  %08x  args:", ebp, eip);
		for (int i = 0 ; i < 5 ; i++)
			printf(" %08x", args[i]);
		printf("\n");
	};

	return 0;
}

static int command_sysinfo(int argc, char **argv) {

	if (_multiboot_magic != MULTIBOOT_BOOTLOADER_MAGIC) {
		printf("Invalid magic number: 0x%x\n", (unsigned) _multiboot_magic);
		return -1;
	}

	if (_multiboot_info->flags & MULTIBOOT_INFO_BOOTDEV)
		printf("boot_device: 0x%08x\n", _multiboot_info->boot_device);

	if (_multiboot_info->flags & MULTIBOOT_INFO_CMDLINE)
		printf("grub cmdline: %s\n", _multiboot_info->cmdline);

	if (_multiboot_info->flags & MULTIBOOT_INFO_MEMORY) {
		printf("low_mem: %d kb\n", _multiboot_info->mem_lower);
		printf("upper_mem: %d kb\n", _multiboot_info->mem_upper);
	}

	if (_multiboot_info->flags & MULTIBOOT_INFO_MEM_MAP) {
		printf("Physical Memory Map:\n");
		for (multiboot_memory_map_t *mmap = (multiboot_memory_map_t *) _multiboot_info->mmap_addr;
				(uint32_t)mmap < _multiboot_info->mmap_addr + _multiboot_info->mmap_length;
				mmap = (multiboot_memory_map_t *)((uint32_t)mmap + mmap->size + sizeof(mmap->size)))
		{
			printf("    0x%08x - 0x%08x  ", mmap->addr_low, mmap->addr_low + mmap->len_low);
			if (mmap->type == MULTIBOOT_MEMORY_AVAILABLE)
				printf("available\n");
			else if (mmap->type == MULTIBOOT_MEMORY_RESERVED)
				printf("reserved\n");
			else
				printf("unknown\n");
		}
	}

	return 0;
}

static int command_info(int argc, char **argv) {
	uint32_t unusable = (1<<20) - mem_lowFree();
	uint32_t used_mem = mem_total() - mem_free() - unusable - mem_lowFree();

	printf("Memory Info:\n");
	printf("total: %d kb\n", mem_total()>>10);
	printf("free: %d kb\n", mem_free()>>10);
	printf("used: %d kb\n", used_mem>>10);
	printf("unusable: %d kb\n", unusable>>10);
	printf("free low mem: %d kb\n", mem_lowFree()>>10);

	return 0;
}

TestProgram user_programs[] = {
	{ "yield", &USER_PROGRAM(yield), &USER_PROGRAM_END(yield) },
	{ "hello", &USER_PROGRAM(hello), &USER_PROGRAM_END(hello) },
	{ "divide_error", &USER_PROGRAM(divide_error), &USER_PROGRAM_END(divide_error) },
	{ "forktest", &USER_PROGRAM(forktest), &USER_PROGRAM_END(forktest) },
	{ "dongu", &USER_PROGRAM(dongu), &USER_PROGRAM_END(dongu) },
	{ "sys_dongu", &USER_PROGRAM(sys_dongu), &USER_PROGRAM_END(sys_dongu) },
	{ "signaltest", &USER_PROGRAM(signaltest), &USER_PROGRAM_END(signaltest) },
	{ "ipctest", &USER_PROGRAM(ipctest), &USER_PROGRAM_END(ipctest) },
	{ "kill", &USER_PROGRAM(kill), &USER_PROGRAM_END(kill) },
	{ "fs", &USER_PROGRAM(fs), &USER_PROGRAM_END(fs) },
};

// FIXME: const
size_t nr_user_programs = sizeof(user_programs)/sizeof(user_programs[0]);

static int command_create(int argc, char **argv) {
	if (argc > 1) {
		for (uint32_t i = 0 ; i < nr_user_programs ; i++) {
			if ( strcmp(user_programs[i].name, argv[1]) == 0) {
				char program_args[100];
				int k = 0;
				for (int j = 1 ; j < argc ; j++) {
					size_t len_argv = strlen(argv[j]);
					strcpy(&program_args[k], argv[j]);
					k+=len_argv;
					program_args[k] = ' ';
					k++;
				}
				program_args[k] = '\0';
				task_create(user_programs[i].addr, program_args, DEFAULT_PRIORITY);
			}
		}
	} else {
		printf("run [program name] [program args]\n");
		printf("programs:\n");
		for (uint32_t i = 0 ; i < nr_user_programs ; i++) {
			printf("    %s\n", user_programs[i].name);
		}
	}
	return 0;
}

static int command_tasks(int argc, char **argv) {
	printf("current task: %8d\n", (task_curr) ? task_curr->id : 0);

	// printf("runnable task: %d\n", __task_runnable_queue.size());
	printf("zombie task: %d\n", task_zombie_list.size());
	printf("\n");

	for (uint32_t i = 0 ; i < task_id_ht.__table_size; i++) {
		TaskIdHashTable_t::node_t *n = task_id_ht.__table[i];
		while (n != NULL) {
			Task* t = n->value();
			printf(" %8d  %20s\n", t->id, task_state_name[t->state]);
			n = n->__next;
		}
	}

	return 0;
}

static int command_taskinfo(int argc, char **argv) {
	extern TaskIdHashTable_t task_id_ht;

	if (argc == 0) {
		printf("%s [task id]\n", argv[0]);
		return -1;
	}

	uint32_t i = atoi(argv[1]);
	Task *t = task_id_ht.get(i);

	if (t == NULL) {
		printf(">> girilen idli task yok\n");
		return -1;
	}

	printf("id:%d  ", t->id);
	printf("parent id:%d  ", (t->parent) ? t->parent->id : 0);
	printf("status:%d  ", t->state);
	printf("run count:%d\n", t->run_count);
	// FIXME: baska bir yontemle bu registerlara ulasilabilmeli
	// printf("eip:0x%08x  esp:0x%08x\n", t->registers_user.eip, t->registers_user.esp);
	printf("time_start: %d\n", t->time_start);
	printf("time_end: %d\n", t->time_end);
	printf("time_kernel: %d\n", t->time_kernel);
	printf("time_user:   %d\n", t->time_user);

	uint32_t pgcount = t->pgdir.count_kernel + t->pgdir.count_user;
	printf("memory usage: %d kb\n", pgcount * 4);
	printf("  user: %d kb\n", t->pgdir.count_user * 4);
	printf("    stack: %d kb\n", t->pgdir.count_stack * 4);
	printf("    program: %d kb\n", t->pgdir.count_program * 4);
	printf("    shared: %d kb\n", t->pgdir.count_shared * 4);
	printf("  kernel: %d kb\n", t->pgdir.count_kernel * 4);
	printf("    kernel stack: 4 kb\n");
	printf("    task struct %d b(?)\n", sizeof(Task));
	printf("    page table vb.\n");
	if (t->childs.size() > 0) {
		printf("childs:\n");
		ChildList_t::iterator it = t->childs.begin();
		for ( ; it != t->childs.end() ; it++) {
			printf("  %8d\n", it->value()->id);
		}
	}

	return 0;
}

static int command_continue(int argc, char **argv) {
	kernel_monitor_running = false;
	return 0;
}

static int command_memdebug(int argc, char **argv) {
	printf("memory debug:\n");

	uint32_t size_tasks_zombie = 0;
	uint32_t size_tasks = 0;
	uint32_t unknown_mem = 0;
	uint32_t ipc_mem = mem_ipc();

	for (uint32_t i = 0 ; i < task_id_ht.__table_size; i++) {
		TaskIdHashTable_t::node_t *n = task_id_ht.__table[i];
		while (n != NULL) {
			Task* t = n->value();
			if (t->state == Task::State_zombie) {
				size_tasks_zombie += t->pgdir.count_kernel << 12;
				size_tasks_zombie += (t->pgdir.count_user - t->pgdir.count_shared) << 12;
				size_tasks_zombie += kmalloc_size(sizeof(Task));
			}
			size_tasks += t->pgdir.count_kernel << 12;
			size_tasks += (t->pgdir.count_user - t->pgdir.count_shared) << 12;
			size_tasks += kmalloc_size(sizeof(Task));

			n = n->__next;
		}
	}

	printf("start free memory: %d k\n", free_memory_start>>10);
	printf("current free memory: %d kb\n", mem_free()>>10);

	printf("zombie tasks: %d kb\n", size_tasks_zombie>>10);
	printf("tasks: %d kb\n", size_tasks>>10);
	printf("ipc: %d kb\n", ipc_mem>>10);

	unknown_mem = free_memory_start - mem_free() - ipc_mem - size_tasks;

	printf("unknown memory: %d kb\n", unknown_mem >> 10);

	return 0;
}
