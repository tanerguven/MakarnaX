#ifndef _INC_BITMAP_H_
#define _INC_BITMAP_H_

#include <asm/x86.h>

inline int bitmap_find_first_zero(unsigned int *bitmap, size_t size) {
	size = size>>2;
 	for (unsigned int i = 0 ; i < size ; i++) {
		if (bitmap[i] == 0xffffffff)
			continue;
		uint32_t b = bit_find_first_zero(bitmap[i]);
		// FIXME: gecici kontrol
		ASSERT(b < 32);
		return 32 * i + b;
	}
	return -1;
}

inline void bitmap_set(unsigned int *bitmap, int pos) {
	bit_set(&bitmap[pos/32], pos % 32);
}

inline void bitmap_reset(unsigned int *bitmap, int pos) {
	bit_reset(&bitmap[pos/32], pos % 32);
}

inline void bitmap_complement(unsigned int *bitmap, int pos) {
	bit_complement(&bitmap[pos/32], pos % 32);
}

inline unsigned char bitmap_test(unsigned int *bitmap, int pos) {
	return bit_test(bitmap[pos/32], pos % 32);
}

#endif /* _INC_BITMAP_H_ */
