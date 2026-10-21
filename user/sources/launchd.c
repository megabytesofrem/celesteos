typedef unsigned int mach_port_name_t;
typedef unsigned int mach_msg_size_t;

#define SYS_MACH_MSG_TRAP (-31)
#define SYS_MACH_PORT_ALLOCATE_TRAP (-33)
#define SYS_MACH_PORT_MAKE_SEND_TRAP (-34)
#define SYS_MACH_PORT_DEALLOCATE_TRAP (-35)

#define MACH_PORT_RIGHT_RECEIVE 1
#define MACH_SEND_MSG 1
#define MACH_RCV_MSG 2

typedef struct mach_msg_header {
    unsigned int msgh_bits;
    mach_msg_size_t msgh_size;
    mach_port_name_t msgh_remote_port;
    mach_port_name_t msgh_local_port;
    unsigned int msgh_id;
} mach_msg_header_t;

_Static_assert(sizeof(mach_msg_header_t) == 20, "unexpected message header size");

static inline long syscall2(long number, long arg1, long arg2) {
    long result;
    __asm__ volatile("syscall"
                     : "=a"(result)
                     : "0"(number), "D"(arg1), "S"(arg2)
                     : "rcx", "r11", "memory");
    return result;
}

static inline long syscall7(long number, long arg1, long arg2, long arg3, long arg4, long arg5,
                            long arg6, long arg7) {
    register long r10 __asm__("r10") = arg4;
    register long r8 __asm__("r8") = arg5;
    register long r9 __asm__("r9") = arg6;
    long result;
    __asm__ volatile("movq %[number], %%rax\n\t"
                     "pushq %[arg7]\n\t"
                     "pushq $0\n\t"
                     "syscall\n\t"
                     "addq $16, %%rsp"
                     : "=&a"(result)
                     : [number] "r"(number), "D"(arg1), "S"(arg2), "d"(arg3), "r"(r10), "r"(r8),
                       "r"(r9), [arg7] "r"(arg7)
                     : "rcx", "r11", "memory");
    return result;
}

void start(void) {
    mach_port_name_t receive_name = 0;
    mach_port_name_t send_name = 0;

    // Allocate a receive right and create a send right for the port
    if (syscall2(SYS_MACH_PORT_ALLOCATE_TRAP, MACH_PORT_RIGHT_RECEIVE, (long)&receive_name) != 0 ||
        syscall2(SYS_MACH_PORT_MAKE_SEND_TRAP, receive_name, (long)&send_name) != 0) {
        goto idle;
    }

    mach_msg_header_t message = {
        .msgh_bits = 0,
        .msgh_size = sizeof(message),
        .msgh_remote_port = send_name,
        .msgh_local_port = 0,
        .msgh_id = 42,
    };

    // Send a message and wait for a reply on the receive port
    long result = syscall7(SYS_MACH_MSG_TRAP, (long)&message, MACH_SEND_MSG | MACH_RCV_MSG,
                           sizeof(message), sizeof(message), receive_name, 0, 0);

    // Wait for the reply and check the message ID
    if (result != 0 || message.msgh_id != 42) {
        goto idle;
    }

    // Deallocate the send and receive rights for the port
    (void)syscall2(SYS_MACH_PORT_DEALLOCATE_TRAP, send_name, 0);
    (void)syscall2(SYS_MACH_PORT_DEALLOCATE_TRAP, receive_name, 0);

idle:
    while (1) {
        __asm__ volatile("pause");
    }
}
