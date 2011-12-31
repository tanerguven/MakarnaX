#ifndef _TIME_H_
#define _TIME_H_

extern uint32_t jiffies;

#define HZ 100

inline uint32_t jiffies_to_seconds() {
	return jiffies / HZ;
}

inline uint32_t jiffies_to_milliseconds() {
	return jiffies * 1000 / HZ;
}

#endif /* _TIME_H_ */
