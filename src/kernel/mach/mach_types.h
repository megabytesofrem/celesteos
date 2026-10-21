#ifndef CELESTEOS_MACH_TYPES_H
#define CELESTEOS_MACH_TYPES_H

#include <stddef.h>
#include <stdint.h>

#include <stdlib/libkernel.h>

typedef uint32_t natural_t;

typedef natural_t mach_port_name_t;
typedef natural_t mach_port_type_t;
typedef natural_t mach_port_right_t;
typedef natural_t mach_port_urefs_t;

/* Port rights */
#define MACH_PORT_RIGHT_SEND 0u
#define MACH_PORT_RIGHT_RECEIVE 1u
#define MACH_PORT_RIGHT_SEND_ONCE 2u
#define MACH_PORT_RIGHT_PORT_SET 3u
#define MACH_PORT_RIGHT_DEAD_NAME 4u

/* Port types */
#define MACH_PORT_TYPE_NONE ((mach_port_type_t)0)
#define MACH_PORT_TYPE(right) ((mach_port_type_t)(UINT32_C(1) << ((right) + 16u)))
#define MACH_PORT_TYPE_SEND MACH_PORT_TYPE(MACH_PORT_RIGHT_SEND)
#define MACH_PORT_TYPE_RECEIVE MACH_PORT_TYPE(MACH_PORT_RIGHT_RECEIVE)
#define MACH_PORT_TYPE_SEND_ONCE MACH_PORT_TYPE(MACH_PORT_RIGHT_SEND_ONCE)
#define MACH_PORT_TYPE_PORT_SET MACH_PORT_TYPE(MACH_PORT_RIGHT_PORT_SET)
#define MACH_PORT_TYPE_DEAD_NAME MACH_PORT_TYPE(MACH_PORT_RIGHT_DEAD_NAME)

#endif // CELESTEOS_MACH_TYPES_H