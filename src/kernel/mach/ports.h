#ifndef CELESTEOS_MACH_PORTS_H
#define CELESTEOS_MACH_PORTS_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#include <kernel/spinlock.h>
#include <kernel/thread.h>

typedef uint32_t mach_port_name_t;
typedef uint32_t mach_port_right_t;

typedef struct mach_kmsg {
    struct mach_kmsg* next;
    struct mach_kmsg* prev;
    uint32_t size;  /* size of the message */
    uint8_t data[]; /* the message data */
} mach_kmsg_t;

/**
 * A Mach IPC port.
 */
typedef struct ipc_port {
    spinlock_t lock;
    uint32_t ref_count; /* reference count for the port */
    bool is_alive;      /* indicates if the port is currently alive */

    /* Message queue */
    mach_kmsg_t* msg_head; /* head of the queue */
    mach_kmsg_t* msg_tail; /* tail of the queue */
    uint32_t msg_count;    /* number of messages in the queue */

    thread_t* blocked_thread; /* thread blocking on the port */
} ipc_port_t;

/**
 * A single entry in the Mach port table.
 */
typedef struct mach_port_entry {
    mach_port_name_t name;   /* the name of the port */
    mach_port_right_t right; /* the associated port right (e.g., send, receive) */
    ipc_port_t* port;
} mach_port_entry_t;

/**
 * The Mach port table, which holds all port entries.
 */
typedef struct mach_port_table {
    mach_port_entry_t* entries;
    uint32_t count;
} mach_port_table_t;

/**
 * The Mach port space, which contains the port table and a lock.
 */
typedef struct mach_port_space {
    spinlock_t lock;
    mach_port_table_t table;
} mach_port_space_t;

#endif  // CELESTEOS_MACH_PORTS_H