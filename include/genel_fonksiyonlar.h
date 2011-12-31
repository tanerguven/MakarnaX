#ifndef GENELFONKSIYONLAR_H_
#define GENELFONKSIYONLAR_H_

#include <types.h>
#include <string.h>

#ifdef __cplusplus

static inline uint32_t roundUp(uint32_t x) {
	return ((x+0x1000-1) & ~(0x1000-1));
}

static inline uint32_t roundDown(uint32_t x) {
	return (x & ~(0x1000-1));
}


static inline uint32_t isRounded(uint32_t x) {
	return !(x & (0xFFF));
}

inline void* roundUp(void* x) {
	return (void*)roundUp((uint32_t)x);
}

inline void* roundDown(void *x) {
	return (void*)roundDown((uint32_t)x);
}

inline uint32_t isRounded(void *x) {
	return isRounded((uint32_t)x);
}

#endif


static inline char isClean(uint8_t* ptr, uint32_t size) {
	uint32_t i;
	for (i = 0 ; i < size ; i++) {
		if (ptr[i] != 0)
			return 0;
	}
	return 1;
}

static inline char isSet(uint8_t* ptr, uint32_t size) {
	uint32_t i;
	for (i = 0 ; i < size ; i++) {
		if (ptr[i] != 0xFF)
			return 0;
	}
	return 1;
}


#ifdef __cplusplus
extern "C" {
#endif

extern uint32_t intToString(char *buf, unsigned int num, unsigned int base);
extern uint32_t str_to_int(const char *s);
extern int atoi(const char *s);
#ifdef __cplusplus
}
#endif

#endif /* GENELFONKSIYONLAR_H_ */
