# CelesteOS
An incredibly small nano/exokernel operating system.

## Architecture
| Directory      | Note                                                         |
| -------------- | -------------------------------------------------------------|
| include/       | Shared headers for kernel and user space                     |
| celesteos/ke/  | Kernel executive (core kernel functionality)                 |
| celesteos/ps/  | Process Subsystem (tasks, threads, scheduler, endpoints)     |
| celesteos/ob/  | Object Manager (kernel object management)                    |
| celesteos/svc/ | System Services (userland service implementations)           |
| lib/           | Small C library used in-kernel                               |

## Design Principles
- The kernel should do absolutely nothing but route IPC and the minimal necessary system management.
- LibOS: The operating system is implemented as a library in userland, providing essential services and abstractions while keeping the kernel minimal.
- Tau: The Tau compiler, used for compiling Tau code to C ahead-of-time. Tau is a sister project to CelesteOS and can be found at https://github.com/megabytesofrem/tau-lang.

## Ideas to explore
- Registry for centralized configuration of settings stored at `/Settings/User` because I despise how UNIX does it with `~/.config`
- Object Manager namespaces e.g `/Devices/Disks/Disk0/Par1`, `/Devices/Disks/Disk0/Par2`

## What works
- [x] Core CPU & Memory Primitives (GDT, IDT, TSS, PMM, Page Map)
- [x] Bitmap and frame allocator for physical memory management in-kernel
- [x] Task control blocks and threads, managed by the kernel's scheduler
- [x] Scheduler and context switching for kernel threads (Round-robin style time-slicing)
- [x] Synchronous Rendezvous IPC (`KeSvcSendMessage`, `KeSvcReplyWait`)

## What needs to be implemented (the boring stuff)
- [ ] ELF loading (`PT_LOAD` segment mapper)
- [ ] Userland bootstrap and server initialization
- [ ] Userland malloc/free implementation: the kernel has no heap allocation
- [ ] EXT2 and EXT4 file system support
- [ ] Userland bring up servers (`root0`, `vfs`, etc)
- [ ] POSIX compatibility layer: standard POSIX APIs, all sandboxed under `/System/posix/`

## Fun stuff to explore later
- [ ] Wire up a fly connectome model and let it loose within a userland server — all over IPC
- [ ] Build a display server, inspired by the X Window System, draw some windows
- [ ] Build a network stack and implement basic networking protocols (TCP/IP, UDP)

## Boot flow
- Kernel setups the CPU features (GDT, IDT, TSS, etc.)
- Kernel setups a bootstrap physical memory manager (PMM) and page map for virtual memory management
- Kernel initializes the task scheduler and creates the initial kernel thread
- Kernel immediately jumps to userspace and executes `root0`
- `root0` is responsible for setting up userland services

## Raw IPC primitives
CelesteOS implements a minimal set of IPC primitives based on L4-style synchronous rendezvous. Its IPC mechanisms are inspired by Mesosphère (https://github.com/Atmosphere-NX/Atmosphere/tree/master).

CelesteOS has the concepts of kernel objects, capabilities, threads, and tasks.

**Kernel object**: An `ObHeader` is the common header for a kernel-managed resource such as a thread, task, IPC endpoint, or page object.

**Object pool**: An `ObObjectPool` tracks objects in fixed-size static backing storage, allowing allocation and reclamation without a kernel heap.

**Tasks**: A task is an abstraction representing a single unit of execution, typically encompassing one or more threads and associated resources. Tasks are the primary entities that interact with the kernel and other tasks via IPC.

**Threads**: A thread is the basic unit of CPU execution within a task. Each thread has its own execution context, including registers and stack, but shares the task's resources such as memory and capabilities.

**Capabilities**: A capability is a generation-checked handle that grants a task specific access rights to a kernel-managed resource. Capabilities are used to control access to threads, tasks, memory regions, IPC endpoints, and other kernel objects.

**IPC Endpoints**: An IPC endpoint is a kernel object that represents a communication channel for inter-process communication. Tasks use IPC endpoints to send and receive messages synchronously.

## Object API
Object pools are initialized with `ObObjectPoolInit` and an `ObOperations` table. Objects embed `ObHeader` as their first member; `ObCreateInPool`, `ObRetain`, and `ObRelease` manage allocation and references.

Subsystem constructors and destructors use the shared object callback signatures:
```c
#include <ob/obheader.h>
#include <ob/obcreation.h>

struct MyObjectData {
    int value;
    char field[64];
};

typedef struct MyObject {
    ObHeader object;

    /* Data that this kobject manages */
    struct MyObjectData data;
} MyObject;

static MyObject g_myObjects[MY_OBJECT_POOL_SIZE];
static ObObjectPool g_myObjectPool;

static void MyObjectCreate(ObHeader *object, ObObjectData data);
static void MyObjectDestroy(ObHeader *object);

static const ObOperations g_myObjectOperations = {
    .name = "my_kobject",
    .create = MyObjectCreate,
    .destroy = MyObjectDestroy,
};

static void MyObjectCreate(ObHeader *object, ObObjectData data) {
    MyObject *myObject = (MyObject *)object;

    myObject->data = *(struct MyObjectData *)data;
}

static void MyObjectDestroy(ObHeader *object) {
    MyObject *myObject = (MyObject *)object;

    /* Perform any necessary cleanup */
    myObject->data.value = 0;
    memset(myObject->data.field, 0, sizeof(myObject->data.field));
}

static void MySubsystemInitialize(void) {
    /* Initialize the static object pool for MyObject */
    ObObjectPoolInit(&g_myObjectPool, g_myObjects, sizeof(MyObject), MY_OBJECT_POOL_SIZE,
                     &g_myObjectOperations);
}
```

## Service Calls

Service-call numbers and message structures are declared in `celesteos/ke/svc.h`; kernel handlers are declared in `celesteos/ke/svc_sys.h`.

Service calls use opaque `KeSvcHandle` values rather than passing thread or task IDs directly.

### 1: `KeSvcSendMessage(target, message, recvBuffer)`
- **Purpose**: The client-side half of L4 Rendezvous IPC. Sends a synchronous IPC message to the target handle and blocks until a reply is received.
- **Registers passed**: Target handle (`RDI`), message (`RSI`), receive buffer (`RDX`)

### 2: `KeSvcReplyWait(client, message, recvBuffer)`
- **Purpose**: The server-side half of L4 Rendezvous IPC. Sends a reply to the client handle and waits for the next IPC message.
- **Registers passed**: Client handle (`RDI`), message (`RSI`), receive buffer (`RDX`)

### 3: `KeSvcVMMap(target, virtAddr, physAddr, flags)`
- **Purpose**: Reserved for mapping a physical page into the target address space; currently returns an error because it is not implemented.
- **Registers passed**: Target handle (`RDI`), virtual address (`RSI`), physical address (`RDX`), flags (`RCX`)

### 4: `KeSvcVMUnmap(target, virtAddr)`
- **Purpose**: Reserved for unmapping a virtual page; currently returns an error because it is not implemented.
- **Registers passed**: Target handle (`RDI`), virtual address (`RSI`)

### 5: `KeSvcCreateThread(target, entryPoint, userStackTop)`
- **Purpose**: Creates a thread in the task referenced by the target handle and queues it for scheduling.
- **Registers passed**: Target handle (`RDI`), entry point (`RSI`), user stack top (`RDX`)

### 6: `KeSvcYieldThread(target)`
- **Purpose**: Yields the CPU to the specified thread handle, or to the scheduler when passed `INVALID_HANDLE`.
- **Registers passed**: Target handle (`RDI`)
