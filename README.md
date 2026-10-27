# CelesteOS
A small operating system developed as a hobby inspired by the L4 microkernel and Mesosphère.

## Architecture
| Directory | Note                                                                |
| --------- | ------------------------------------------------------------------- |
| include/  | stores shared headers for both kernel and user space                |
| kernel/   | stores the core microkernel and the minimum systems in kernel space |
| servers/  | stores userspace servers (IPC communication)                        |
| lib/      | stores the tiny libc used in-kernel                                 |

## Checklist

### Phase 1: Core CPU & Memory Primitives
- [x] GDT, IDT, and TSS setup (Ring 0 / Ring 3 descriptors)
- [x] Physical Memory Allocator (Bitmap / Frame allocator for 4KB pages)
- [x] Page Table Manager (`pmap_map` / `pmap_unmap` for PML4 tables)
- [ ] Local APIC & IOAPIC initialization (Disable legacy 8259 PIC)

### Phase 2: Execution & Traps
- [x] Assembly Trap Stub (`syscall` / `sysret` entry point & ISR wrappers)
- [x] Task Control Blocks (TCB) & Thread Scheduler (Round-robin context switching)

### Phase 3: L4 Primitives & IPC
- [x] Capability Table & Handle Array management
- [-] Synchronous Rendezvous IPC (`sys_ipc_send`, `sys_ipc_recv`, `sys_ipc_call`)
- [-] Userland Page Fault (`#PF`) & Hardware IRQ delegation to IPC

### Phase 4: Userland Bootstrap & Servers
- [ ] Static 64-bit ELF Loader (`PT_LOAD` segment mapper)
- [ ] Spawn Root Task / Pager (Thread 1) into Ring 3
- [ ] Core Userland Servers (`alloc_server`, `vfs_server`, `driver_server`)

## IPC
CelesteOS implements a minimal set of IPC primitives based on L4-style synchronous rendezvous. Its IPC mechanisms are inspired by Mesosphère (https://github.com/Atmosphere-NX/Atmosphere/tree/master).

CelesteOS has the concepts of kobjects (kernel objects), capabilities, threads and tasks. 

**Kobject**: A kobject is the general notion of a kernel-managed resource (whether that is a thread, task, IPC endpoint, page object, etc.)

**KObject Pool**: A kobject pool is a pool that tracks allocated kernel objects with static backing storage, allowing efficient allocation and deallocation of kernel objects in ring 0.

**Tasks**: A task is an abstraction representing a single unit of execution, typically encompassing one or more threads and associated resources. Tasks are the primary entities that interact with the kernel and other tasks via IPC.

**Threads**: A thread is the basic unit of CPU execution within a task. Each thread has its own execution context, including registers and stack, but shares the task's resources such as memory and capabilities.

**Capabilities**: A capability is a token or reference that grants a task certain rights to interact with a kernel-managed resource (kobject). Capabilities are used to control access to threads, tasks, memory regions, IPC endpoints, and other kernel objects.

**IPC Endpoints**: An IPC endpoint is a kernel object that represents a communication channel for inter-process communication. Tasks use IPC endpoints to send and receive messages synchronously.

## KObject API
Each subsystem that manages kernel objects must implement a set of operations defined in `kobject_ops_t`, which includes the destructor for the kernel object and any other relevant operations.

Below is an example documenting the kobject API.
```c
#include <kernel/ipc/kobject.h>

struct my_kobject_data {
    int value;
    char field[64];
}; 

typedef struct my_kobject {
    kobject_t obj;

    /* Data that this kobject manages */
    struct my_kobject_data data;
} my_kobject_t;

my_kobject_t g_my_kobject[MY_KOBJECT_POOL_SIZE];
kobject_pool_t g_my_kobject_pool;

static void my_kobject_construct(kobject_t *obj, kobject_data_t data);
static void my_kobject_destroy(kobject_t *obj);

static const kobject_ops_t my_kobject_ops = {
    .name = "my_kobject",
    .construct = my_kobject_construct,
    .destroy = my_kobject_destroy,
};

static void my_kobject_construct(kobject_t *obj, kobject_data_t data) {
    my_kobject_t *my_obj = (my_kobject_t *)obj;

    my_obj->data = *(struct my_kobject_data *)data;
}

static void my_kobject_destroy(kobject_t *obj) {
    my_kobject_t *my_obj = (my_kobject_t *)obj;

    /* Perform any necessary cleanup */
    my_obj->data.value = 0;
    memset(my_obj->data.field, 0, sizeof(my_obj->data.field));
}

static void my_subsystem_sys_init(void) {
    /* Initialize the static object pool for my_kobject_t */
    kobject_pool_init(&g_my_kobject_pool, 
                       g_my_kobject,
                       sizeof(my_kobject_t),
                       MY_KOBJECT_POOL_SIZE,
                       &my_kobject_ops);
}
```

## Service Calls

Service calls are implemented in `kernel/svc.h` as syscalls; however they are tiny and only communicate with userland and IPC mechanisms.

Service calls are similar to L4 IPC calls, with the distinct difference that we never pass thread or task IDs directly; instead, we always use opaque handles (`handle_t`) to refer to kernel-managed resources (within the target context).

### 1: `sys_svc_send_msg(target, message, recv_buffer)`
- **Purpose**: The client-side half of L4 Rendezvous IPC. Sends a synchronous IPC message to the target handle and blocks until a reply is received.
- **Registers passed**: Target handle (`RDI`), message (`RSI`), receive buffer (`RDX`)

### 2: `sys_svc_reply_wait(client, message, recv_buffer)`
- **Purpose**: The server-side half of L4 Rendezvous IPC. Sends a reply to the client handle and waits for the next IPC message.
- **Registers passed**: Client handle (`RDI`), message (`RSI`), receive buffer (`RDX`)

### 3: `sys_svc_vm_map(target, virt_addr, phys_addr, flags)`
- **Purpose**: Maps a physical page into the virtual address space (page table) of the target.
- **Registers passed**: Target handle (`RDI`), virtual address (`RSI`), physical address (`RDX`), flags (`RCX`)

### 4: `sys_svc_vm_unmap(target, virt_addr)`
- **Purpose**: Unmaps a virtual page from the virtual address space (page table) of the target.
- **Registers passed**: Target handle (`RDI`), virtual address (`RSI`)

### 5: `sys_svc_thread_create(target, entry_point, user_stack_top)`
- **Purpose**: Creates a new thread in the static thread pool `g_threads`, sets up its kernel and user context, and assigns it to the target handle.
- **Registers passed**: Target handle (`RDI`), entry point (`RSI`), user stack top (`RDX`)

### 6: `sys_svc_thread_yield(target)`
- **Purpose**: Yields the CPU from the current thread to the specified target handle. Triggers a manual schedule pass. If `target` is passed, it donates the current thread's time slice to the target (via `scheduler_yield_to`), otherwise it uses a standard round-robin `schedule()`.
- **Registers passed**: Target handle (`RDI`)

