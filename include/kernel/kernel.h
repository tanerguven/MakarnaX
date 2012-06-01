#ifndef _KERNEL_H_
#define _KERNEL_H_

#define _KERNEL_SRC_

#include <types.h>

// panic.cpp
extern void __panic(const char *msg, const char* file, int line);
extern void __panic_assert(const char* file, int line, const char* d);
extern void __panic_assert3(const char* file, int line, const char *c_a, uint32_t v_a,
							const char *op, const char *c_b, uint32_t v_b);
#define PANIC(msg) __panic(msg, __FILE__, __LINE__)
#define ASSERT(b) ((b) ? (void)0 : __panic_assert(__FILE__, __LINE__, #b))
#define ASSERT3(a, op, b) \
	((a op b) ? (void)0 : __panic_assert3(__FILE__, __LINE__, #a, a, #op, #b, b))

// print.c
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

// spinlock.cpp
struct spinlock {
	uint32_t locked;
};
asmlink void pushcli();
// FIXME: popif yerine popcli kullan
asmlink void popif();
#define popcli popif
asmlink void spinlock_init(struct spinlock *l);
asmlink void spinlock_acquire(struct spinlock* l);
asmlink void spinlock_release(struct spinlock* l);

#include "debug.h"

#endif /* _KERNEL_H_ */
