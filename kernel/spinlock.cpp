
#include <kernel/kernel.h>
#include <asm/x86.h>

static inline uint32_t xchg(volatile uint32_t *addr, uint32_t newval) {
	uint32_t result;

	asm volatile("lock; xchgl %0, %1"
				 : "+m" (*addr), "=a" (result)
				 : "1" (newval)
				 : "cc");
	return result;
}

asmlink void spinlock_init(struct spinlock *l) {
	ASSERT_DTEST(l != NULL);
	l->locked = 0;
}

asmlink void spinlock_acquire(struct spinlock* l) {
	ASSERT_DTEST(l != NULL);
	pushcli();
	while (xchg(&l->locked, 1) == 0)
		/* bekle */;
}

asmlink void spinlock_release(struct spinlock* l) {
	ASSERT_DTEST(l != NULL);
	xchg(&l->locked, 0);
	popif();
}


// FIXME: cpu'ya ozel degiskenler, 1 cpu oldugu icin simdilik boyle
int n_stackif = 0;
static uint32_t stackif;

asmlink void pushcli() {
	uint32_t eflags = eflags_read();

	cli();
	if (eflags & FL_IF)
		bit_set(&stackif, n_stackif);
	else
		bit_reset(&stackif, n_stackif);
	n_stackif++;
	if (n_stackif > 31)
		PANIC("--");
}

asmlink void popif() {
	uint8_t r;

	cli();
	if (--n_stackif < 0)
		PANIC("popif sayisi, pushdan fazla");

	r = bit_test(stackif, n_stackif);
	if (r)
		sti();
}

asmlink void pushsti() {
	uint32_t eflags = eflags_read();

	cli();
	if (eflags & FL_IF)
		bit_set(&stackif, n_stackif);
	else
		bit_reset(&stackif, n_stackif);
	n_stackif++;
	if (n_stackif > 31)
		PANIC("--");
	sti();
}
