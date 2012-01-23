#ifndef _KERNEL_H_
#define _KERNEL_H_

// kmalloc.cpp
extern void* kmalloc(size_t size);
extern void kfree(void* ptr);
extern size_t kmalloc_size(size_t size);

#endif /* _KERNEL_H_ */
