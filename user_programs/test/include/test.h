#ifndef _TEST_H_
#define _TEST_H_

static inline void __panic_assert(const char* file, int line, const char* d) {
	printf("user panic: %s:%d\n", file, line);
	printf("assertion failed: %s\n", d);
	exit(0);
}

static inline void __panic_assert3(const char* file, int line, const char *c_a, uint32_t v_a,
								   const char *op, const char *c_b, uint32_t v_b) {
	printf("user panic: %s:%d\n", file, line);
	printf("assertion failed: %s %s %s\n", c_a, op, c_b);
	printf("%s=%08x, %s=%08x\n", c_a, v_a, c_b, v_b);
	exit(0);
}

#define PANIC(msg) __panic(msg, __FILE__, __LINE__)
#define ASSERT(b) ((b) ? (void)0 : __panic_assert(__FILE__, __LINE__, #b))
#define ASSERT3(a, op, b) \
	((a op b) ? (void)0 : __panic_assert3(__FILE__, __LINE__, #a, a, #op, #b, b))

#define assertEquals(expected, actual) do { \
	if (expected != actual) \
		printf("assertion failed! %s:%d\n  "#expected" (%d) == "#actual" (%d)\n", \
			    __FILE__, __LINE__, expected, actual); \
} while (0)

#define assertNotLess(value, limit) do {			\
	  if (value < limit) \
		  printf("assertion failed! %s:%d\n  "#limit" (%d) < "#value" (%d)\n", \
				 __FILE__, __LINE__, limit, value);	\
} while (0)

#define assertNotEquals(notExpected, actual) do { \
	if (notExpected == actual) \
		printf("assertion failed! %s:%d\n  "#notExpected" (%d) != "#actual" (%d)\n", \
			   __FILE__, __LINE__, notExpected, actual); \
} while (0)

#endif /* _TEST_H_ */
