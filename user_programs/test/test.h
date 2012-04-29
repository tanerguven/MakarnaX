#ifndef _TEST_H_
#define _TEST_H_

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
