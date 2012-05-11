#ifndef _KERNEL_DEBUG_H
#define _KERNEL_DEBUG_H

#define __KERNEL_DEBUG_interrupt_check  1
#define __KERNEL_DEBUG_test 1

#define debug_test(level) (__KERNEL_DEBUG_##level == 1)

#if __KERNEL_DEBUG_test == 1
# define ASSERT_DTEST(p) ASSERT(p)
#else
# define ASSERT_DTEST(p) /* */
#endif

#if __KERNEL_DEBUG_interrupt_check == 1
# define ASSERT_int_disable() ASSERT(!(eflags_read() & FL_IF))
#else
# define ASSERT_int_disable() /* */
#endif

// TODO: fonksiyonlarin kac kere cagrildigini sayan birsey

#endif /* _KERNEL_DEBUG_H */
