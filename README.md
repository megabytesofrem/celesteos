# CelesteOS
A small operating system developed as a hobby based on the L4 microkernel.

## Architecture
| Directory | Note                                                                |
| --------- | ------------------------------------------------------------------- |
| include/  | stores shared headers for both kernel and user space                |
| kernel/   | stores the core microkernel and the minimum systems in kernel space |
| servers/  | stores userspace servers (IPC communication)                        |
| lib/      | stores the tiny libc used in-kernel                                 |

CelesteOS is a microkernel: as much functionality as possible should be moved to user space;
the kernel should be focused on the bare minimum functionality required and everything else should be a server: init_server, fs_server, alloc_server, etc.

The IPC mechanism operates on thread control blocks (TCBs), which store the state and context of each thread, which themself contain a task containing a cr3 link to the page table, and a table of capabilities for the task in question.


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

## Scheduler

The single-CPU round-robin interface is scaffolded in
[`kernel/ipc/scheduler.h`](kernel/ipc/scheduler.h); there is no scheduler implementation
or timer-driven context switching yet. The interface covers ready queues, IPC
blocking/wakeup, yielding, and tick accounting. Its operations require interrupts
disabled, and IRQ return support must be added before enabling preemption.

## Service Calls
CelesteOS only provides the bare minimum service calls needed to support userland servers and basic IPC functionality. This includes service calls for thread management, memory management, and inter-process communication.

Service calls are implemented in `kernel/svc.h` as syscalls; however they are tiny and only communicate with userland and IPC mechanisms.

### 1: `svc_send_msg(target_tid, message, recv_buffer)`
- **Purpose**: The client-side half of L4 Rendezvous IPC. Sends a synchronous IPC message to the target thread and blocks until a reply is received.
- **Registers passed**: Target thread (`RDI`), message (`RSI`), receive buffer (`RDX`)

### 2: `svc_reply_wait(client_tid, message, recv_buffer)`
- **Purpose**: The server-side half of L4 Rendezvous IPC. Sends a reply to the client thread and waits for the next IPC message.
- **Registers passed**: Client thread (`RDI`), message (`RSI`), receive buffer (`RDX`)

### 3: `svc_vm_map(target_task, virt_addr, phys_addr, flags)`
- **Purpose**: Maps a physical page into the virtual address space (page table) of the target task.
- **Registers passed**: Target task (`RDI`), virtual address (`RSI`), physical address (`RDX`), flags (`RCX`)

### 4: `svc_vm_unmap(target_task, virt_addr)`
- **Purpose**: Unmaps a virtual page from the virtual address space (page table) of the target task.
- **Registers passed**: Target task (`RDI`), virtual address (`RSI`)

### 5: `svc_thread_create(target_task, entry_point, user_stack_top)`
- **Purpose**: Creates a new thread in the static thread pool `g_threads`, sets up its kernel and user context, and assigns it to the target task.
- **Registers passed**: Target task (`RDI`), entry point (`RSI`), user stack top (`RDX`)

### 6: `svc_thread_yield(target_tid)`
- **Purpose**: Yields the CPU from the current thread to the specified target thread. Triggers a manual schedule pass. If `target_tid` is passed, it donates the current thread's time slice to the target (via `scheduler_yield_to`), otherwise it uses a standard round-robin `schedule()`.
- **Registers passed**: Target thread (`RDI`)