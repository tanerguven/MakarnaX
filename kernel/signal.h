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
