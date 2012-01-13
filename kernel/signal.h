#ifndef _SIGNAL_H_
#define _SIGNAL_H_

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
