#ifndef _KERNEL_CONFIGURATION_H
#define _KERNEL_CONFIGURATION_H

/* memory/segmentation */
#define __CONF_segmentation_off 1
#define __CONF_segmentation_higher_kernel 2
#define __CONF_segmentation_lower_kernel 3

#include "../../kernel_conf.h"


#ifndef __CONF_segmentation
# error __CONF_segmentation tanimlanmamis
#endif

#ifndef __CONF_CPU_invlpg
# warning __CONF_CPU_invlpg not defined
#endif

#endif /* _KERNEL_CONFIGURATION_H */
