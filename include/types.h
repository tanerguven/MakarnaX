#ifndef TYPEDEFS_H
#define TYPEDEFS_H

#include <stddef.h>

typedef unsigned int   uint32_t;
typedef          int   int32_t;
typedef unsigned short uint16_t;
typedef          short int16_t;
typedef unsigned char  uint8_t;
typedef          char  int8_t;

typedef char* ptr_t;
typedef uint32_t addr_t;

#ifdef __cplusplus
#define asmlink extern "C"
# else
#define asmlink extern
#endif

// FIXME: newlib types.h
#ifndef _SYS_TYPES_H
typedef int key_t;
typedef short ino_t;
typedef int off_t;
typedef short dev_t;
#endif
#endif /* TYPEDEFS_H */
