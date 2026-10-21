#ifndef CELESTEOS_STUB_H
#define CELESTEOS_STUB_H

#include <libkernel.h>

/* Mark a function as unimplemented */
#define UNIMPLEMENTED                                                                              \
    do {                                                                                           \
        klog(KLOG_WARN, " UNIMPLEMENTED: %s:%s (%d)\n", __FILE__, __FUNCTION__, __LINE__);         \
    } while (0)

/* Mark a function as needing a fix */
#define FIXME(x)                                                                                   \
    do {                                                                                           \
        klog(KLOG_WARN, " FIXME: %s:%s (%d): " x "\n", __FILE__, __FUNCTION__, __LINE__);          \
    } while (0)

#endif // CELESTEOS_STUB_H