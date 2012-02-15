#ifndef _KERNEL_H_
#define _KERNEL_H_

#include "panic.h"
#include <stdio.h>

// kmalloc.cpp
extern void* kmalloc(size_t size);
extern void kfree(void* ptr);
extern size_t kmalloc_size(size_t size);

// task_exit.cpp
asmlink void do_exit(int);




// FIXME: bunlar baska bir yere tasinabilir
#include "task.h"

inline uint32_t user_to_kernel(uint32_t base, uint32_t limit, int rw) {
	int perm = (rw) ? PTE_W | PTE_U : PTE_U;
	if ( task_curr->pgdir.verify_user_addr((const void*)base, limit, perm) < 0 ) {
		printf(">> user addr not verified: 0x%08x - 0x%08x\n", base,
			   base+limit);
		do_exit(111);
		PANIC("user_to_kernel");
	}

	return uaddr2kaddr(base);
}

#endif /* _KERNEL_H_ */
