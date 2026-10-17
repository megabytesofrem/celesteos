#ifndef CELESTEOS_MACH_TYPES_H
#define CELESTEOS_MACH_TYPES_H

#include <stddef.h>
#include <stdint.h>

#include <libkernel.h>

typedef uint32_t mach_port_name_t;
typedef uint32_t mach_port_right_t;

/**
 * STUB out functions we have yet to implement.
 */
#define STUB(x)                                                                                    \
	do {                                                                                       \
		klog(KLOG_WARN, " STUB: " x);                                                      \
		return KERN_SUCCESS;                                                               \
	} while (0)

#endif // CELESTEOS_MACH_TYPES_H