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

/**
 * // gibi durumlarda buf = "" olabilir
 * @return path'de kalinan yer
 */
static inline int parse_path_i(const char *path, int i, char buf[256]) {
	int j = 0;

	while (path[i] != '\0') {
		/* basta olmayan / yerine \0 */
		if ( j > 0 && path[i] == '/' ) {
			buf[j] = '\0';
			return i+1;
		}
		/* bastaki /'lari yoksay */
		if (path[i] != '/') {
			buf[j] = path[i];
			j++;
		}
		i++;
	}
	buf[j] = '\0';
	return 0;
}

#ifdef __cplusplus
extern "C" {
#endif

// lib/genelfonksiyonlar.c
extern uint32_t intToString(char *buf, unsigned int num, unsigned int base);
extern uint32_t str_to_int(const char *s);
extern int atoi(const char *s);
extern void parse_cmd(char *cmd, int *argc, char *argv[11]);

#ifdef __cplusplus
}
#endif

#endif /* GENELFONKSIYONLAR_H_ */
