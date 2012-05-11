#ifndef _KERNEL_CONF_H_
#define _KERNEL_CONF_H_

#include <kernel/configuration.h>


#define __CONF_segmentation __CONF_segmentation_higher_kernel
#define __CONF_CPU_invlpg 1

#define __CONF_semaphore_single_cpu_optimization 1

// ekran ciktilarini COM1'e yonlendir ve COM1'den klavye girdisi al
#define __CONF_use_COM1_io

#endif /* _KERNEL_CONF_H_ */
