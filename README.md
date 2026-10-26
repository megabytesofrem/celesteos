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
- [X] Physical Memory Allocator (Bitmap / Frame allocator for 4KB pages)
- [X] Page Table Manager (`pmap_map` / `pmap_unmap` for PML4 tables)
- [ ] Local APIC & IOAPIC initialization (Disable legacy 8259 PIC)

### Phase 2: Execution & Traps
- [X] Assembly Trap Stub (`syscall` / `sysret` entry point & ISR wrappers)
- [ ] Task Control Blocks (TCB) & Thread Scheduler (Round-robin context switching)

### Phase 3: L4 Primitives & IPC
- [ ] Capability Table & Handle Array management
- [ ] Synchronous Rendezvous IPC (`sys_ipc_send`, `sys_ipc_recv`, `sys_ipc_call`)
- [ ] Userland Page Fault (`#PF`) & Hardware IRQ delegation to IPC

### Phase 4: Userland Bootstrap & Servers
- [ ] Static 64-bit ELF Loader (`PT_LOAD` segment mapper)
- [ ] Spawn Root Task / Pager (Thread 1) into Ring 3
- [ ] Core Userland Servers (`alloc_server`, `vfs_server`, `driver_server`)

## IPC
CelesteOS implements a minimal set of IPC primitives based on L4-style synchronous rendezvous. Its IPC mechanisms are inspired by Mesosphère (https://github.com/Atmosphere-NX/Atmosphere/tree/master).

CelesteOS has the concepts of kobjects (kernel objects), capabilities, threads and tasks. 

**Kobject**: A kobject is the general notion of a kernel-managed resource (whether that is a thread, task, IPC endpoint, page object, etc.)

**Tasks**: A task is an abstraction representing a single unit of execution, typically encompassing one or more threads and associated resources. Tasks are the primary entities that interact with the kernel and other tasks via IPC.

**Threads**: A thread is the basic unit of CPU execution within a task. Each thread has its own execution context, including registers and stack, but shares the task's resources such as memory and capabilities.

**Capabilities**: A capability is a token or reference that grants a task certain rights to interact with a kernel-managed resource (kobject). Capabilities are used to control access to threads, tasks, memory regions, IPC endpoints, and other kernel objects.

**IPC Endpoints**: An IPC endpoint is a kernel object that represents a communication channel for inter-process communication. Tasks use IPC endpoints to send and receive messages synchronously.

## Services

Services are implemented in `kernel/svc.h` as syscalls; however they are tiny and only communicate with userland and IPC mechanisms.

Services are similar to L4 IPC calls, with the distinct difference that we never pass thread or task IDs directly; instead, we always use opaque handles (`handle_t`) to refer to kernel-managed resources (within the target context).

### 1: `svc_send_msg(target, message, recv_buffer)`
- **Purpose**: The client-side half of L4 Rendezvous IPC. Sends a synchronous IPC message to the target thread and blocks until a reply is received.
- **Registers passed**: Target handle (`RDI`), message (`RSI`), receive buffer (`RDX`)

### 2: `svc_reply_wait(client, message, recv_buffer)`
- **Purpose**: The server-side half of L4 Rendezvous IPC. Sends a reply to the client thread and waits for the next IPC message.
- **Registers passed**: Client handle (`RDI`), message (`RSI`), receive buffer (`RDX`)

### 3: `svc_vm_map(target, virt_addr, phys_addr, flags)`
- **Purpose**: Maps a physical page into the virtual address space (page table) of the target.
- **Registers passed**: Target handle (`RDI`), virtual address (`RSI`), physical address (`RDX`), flags (`RCX`)

### 4: `svc_vm_unmap(target, virt_addr)`
- **Purpose**: Unmaps a virtual page from the virtual address space (page table) of the target.
- **Registers passed**: Target handle (`RDI`), virtual address (`RSI`)

- ### 5: `svc_thread_create(target, entry_point, user_stack_top)`
- **Purpose**: Creates a new thread in the static thread pool `g_threads`, sets up its kernel and user context, and assigns it to the target.
- **Registers passed**: Target handle (`RDI`), entry point (`RSI`), user stack top (`RDX`)

### 6: `svc_thread_yield(target)`
- **Purpose**: Yields the CPU from the current thread to the specified target thread. Triggers a manual schedule pass. If `target` is passed, it donates the current thread's time slice to the target (via `scheduler_yield_to`), otherwise it uses a standard round-robin `schedule()`.
- **Registers passed**: Target handle (`RDI`)