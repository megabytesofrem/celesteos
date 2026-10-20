#ifndef CELESTEOS_MACH_MESSAGE_H
#define CELESTEOS_MACH_MESSAGE_H

#include <stddef.h>
#include <stdint.h>

#include <mach/kern_return.h>
#include <mach/mach_types.h>

/*
 * Sources referenced:
 * https://web.mit.edu/darwin/src/modules/xnu/osfmk/man/mach_msg.html
 * https://github.com/apple/darwin-xnu/tree/main/osfmk/mach
 *
 * With assistance from Gemini.
 */

#define MACH_PORT_NULL 0
#define MACH_PORT_DEAD ((mach_port_name_t)~0)

#define MACH_MSG_OPTION_NONE 0x00000000

/*
 * Mach message options.
 */
#define MACH_SEND_MSG 0x00000001
#define MACH_RCV_MSG 0x00000002
#define MACH_RCV_LARGE 0x00000004          /* receive large message */
#define MACH_RCV_LARGE_IDENTITY 0x00000008 /* receive large message identity */
#define MACH_SEND_TIMEOUT 0x00000010       /* timeout applies to send */
#define MACH_SEND_OVERRIDE 0x00000020      /* priority override for sending messages */
#define MACH_SEND_INTERRUPT 0x00000040     /* we shouldn't restart interrupted sends */
#define MACH_SEND_NOTIFY 0x00000080

#define MACH_MSG_STRICT_REPLY 0x00000200
#define MACH_RCV_TIMEOUT 0x00000100   /* timeout applies to receive */
#define MACH_RCV_NOTIFY 0x00000000    /* legacy deprecated */
#define MACH_RCV_INTERRUPT 0x00000400 /* we shouldn't restart interrupted receives */
#define MACH_RCV_VOUCHER 0x00000800
#define MACH_RCV_OVERWRITE 0x00000000 /* deprecated in XNU, so we do too */
#define MACH_RCV_GUARDED_DESC 0x00001000
#define MACH_RCV_SYNC_WAIT 0x00004000
#define MACH_RCV_SYNC_PEEK 0x00008000
#define MACH_SEND_ALWAYS 0x00010000
#define MACH_SEND_TRAILER 0x00020000      /* include a trailer with the message */
#define MACH_SEND_NOIMPORTANCE 0x00040000 /* send without importance */
#define MACH_SEND_NODENAP MACH_SEND_NOIMPORTANCE
#define MACH_SEND_IMPORTANCE 0x00080000 /* send with importance */
#define MACH_SEND_SYNC_OVERRIDE 0x00100000
#define MACH_SEND_PROPAGATE_QOS 0x00200000
#define MACH_SEND_SYNC_USE_THRPRI MACH_SEND_PROPAGATE_QOS
#define MACH_SEND_KERNEL 0x00400000 /* send from kernel space */
#define MACH_SEND_SYNC_BOOTSTRAP_CHECKIN 0x00800000

/* Header Bitmask Constants */
#define MACH_MSGH_BITS_ZERO 0x00000000
#define MACH_MSGH_BITS_REMOTE_MASK 0x000000ff
#define MACH_MSGH_BITS_LOCAL_MASK 0x0000ff00
#define MACH_MSGH_BITS_COMPLEX 0x80000000

#define MACH_MSGH_BITS(remote, local) ((remote) | ((local) << 8))

/* Return Codes */
#define MACH_MSG_SUCCESS 0x00000000

typedef struct {
    uint32_t msgh_bits;                /* flags */
    uint32_t msgh_size;                /* total size of the message */
    mach_port_name_t msgh_remote_port; /* destination port right */
    mach_port_name_t msgh_local_port;  /* reply port right */
    uint32_t msgh_id;                  /* message identifier */
} mach_msg_header_t;

typedef natural_t mach_msg_type_name_t;

/*
 * Port rights for Mach messages.
 */
#define MACH_MSG_TYPE_MOVE_RECEIVE 16   /* move receive right */
#define MACH_MSG_TYPE_MOVE_SEND 17      /* move send right */
#define MACH_MSG_TYPE_MOVE_SEND_ONCE 18 /* move send-once right */
#define MACH_MSG_TYPE_COPY_SEND 19      /* copy send right */
#define MACH_MSG_TYPE_MAKE_SEND 20      /* make send right */
#define MACH_MSG_TYPE_MAKE_SEND_ONCE 21 /* make send-once right */

#define MACH_MSG_TYPE_PORT_SEND MACH_MSG_TYPE_MOVE_SEND
#define MACH_MSG_TYPE_PORT_SEND_ONCE MACH_MSG_TYPE_MOVE_SEND_ONCE
#define MACH_MSG_TYPE_PORT_RECEIVE MACH_MSG_TYPE_MOVE_RECEIVE

/* Mach message return type */
typedef natural_t mach_msg_return_t;

/*
 * Return codes for Mach message send and receive operations.
 */
#define MACH_SEND_IN_PROGRESS 0x10000001   /* send is in progress */
#define MACH_SEND_INVALID_DATA 0x10000002  /* send: invalid data */
#define MACH_SEND_INVALID_DEST 0x10000003  /* send: invalid destination */
#define MACH_SEND_TIMED_OUT 0x10000004     /* send: timed out */
#define MACH_SEND_INTERRUPTED 0x10000005   /* send: interrupted */
#define MACH_SEND_MSG_TOO_SMALL 0x10000006 /* send: message too small */

#define MACH_RCV_IN_PROGRESS 0x20000001  /* receive is in progress */
#define MACH_RCV_INVALID_NAME 0x20000002 /* receive: invalid name */
#define MACH_RCV_TIMED_OUT 0x20000003    /* receive: timed out */
#define MACH_RCV_TOO_LARGE 0x20000004    /* receive: message too large */
#define MACH_RCV_INTERRUPTED 0x20000005  /* receive: interrupted */
#define MACH_RCV_PORT_DIED 0x20000006    /* receive: port died */

typedef uint32_t mach_msg_size_t;
typedef uint32_t mach_msg_option_t;
typedef uint32_t mach_msg_timeout_t;

/**
 * Send and/or receive a Mach message.
 *
 * @param msg The message header to send or receive.
 * @param option The message options.
 * @param send_size The size of the message to send.
 * @param rcv_size The size of the buffer to receive the message.
 * @param rcv_name The name of the receive port.
 * @param timeout The send/receive timeout.
 * @param notify The notification port for send-once rights.
 * @return The result of the send/receive operation.
 */
mach_msg_return_t mach_msg(mach_msg_header_t *msg, mach_msg_option_t option,
                           mach_msg_size_t send_size, mach_msg_size_t rcv_size,
                           mach_port_name_t rcv_name, mach_msg_timeout_t timeout,
                           mach_port_name_t notify);

/**
 * Send a Mach message.
 *
 * Resolves `msg->msgh_remote_port` in the current tasks space, copies the payload into a
 * `mach_kmsg_t` and calls `ipc_port_enqueue_kmsg` to enqueue the message to the destination port.
 *
 * @param msg The message header to send.
 * @param option The send options.
 * @param send_size The size of the message to send.
 * @param timeout The send timeout.
 * @return The result of the send operation.
 */
mach_msg_return_t mach_msg_send(mach_msg_header_t *msg, mach_msg_option_t option,
                                mach_msg_size_t send_size, mach_msg_timeout_t timeout);

/**
 * Receive a Mach message.
 *
 * Resolves `rcv_name` in the current task's space, calls `ipc_port_dequeue_kmsg` to dequeue a
 * message from the receive port, and copies the payload into the provided message buffer (or blocks
 * the calling thread until a message is available).
 *
 * @param msg The message header to receive into.
 * @param option The receive options.
 * @param rcv_size The size of the buffer to receive the message.
 * @param rcv_name The name of the receive port.
 * @param timeout The receive timeout.
 * @return The result of the receive operation.
 */
mach_msg_return_t mach_msg_receive(mach_msg_header_t *msg, mach_msg_option_t option,
                                   mach_msg_size_t rcv_size, mach_port_name_t rcv_name,
                                   mach_msg_timeout_t timeout);

#endif // CELESTEOS_MACH_MESSAGE_H