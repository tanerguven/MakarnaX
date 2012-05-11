#ifndef _KERNEL_H_
#define _KERNEL_H_

#include <types.h>

extern void __panic(const char *msg, const char* file, int line);
extern void __panic_assert(const char* file, int line, const char* d);

#define PANIC(msg) __panic(msg, __FILE__, __LINE__)
#define ASSERT(b) ((b) ? (void)0 : __panic_assert(__FILE__, __LINE__, #b))

asmlink int __kernel_print(const char *fmt, ...);

#define print_info(args...) __kernel_print(args)
#define print_warning(args...) __kernel_print(args)
#define print_error(args...) __kernel_print(args)

// kmalloc.cpp
extern void* kmalloc(size_t size);
extern void kfree(void* ptr);
extern size_t kmalloc_size(size_t size);

// task_exit.cpp
asmlink void do_exit(int);

// syscall.cpp
extern uint32_t user_to_kernel_check(uint32_t base, uint32_t limit, int rw);

#include "debug.h"

#endif /* _KERNEL_H_ */
