#include <kernel/kernel.h>
#include <asm/x86.h>

#include "task.h"

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

asmlink void pushcli() {
	uint32_t eflags = eflags_read();

	cli();
	if (task_curr->n_cli++ == 0)
		task_curr->before_pushcli = eflags & FL_IF;
}

asmlink void popcli() {
	cli();
	if (--task_curr->n_cli < 0)
		PANIC("popif sayisi, pushdan fazla");

	if (task_curr->n_cli == 0 && task_curr->before_pushcli)
		sti();
}
