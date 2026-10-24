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
- [ ] Physical Memory Allocator (Bitmap / Frame allocator for 4KB pages)
- [ ] Page Table Manager (`pmap_map` / `pmap_unmap` for PML4 tables)
- [ ] Local APIC & IOAPIC initialization (Disable legacy 8259 PIC)

### Phase 2: Execution & Traps
- [ ] Assembly Trap Stub (`syscall` / `sysret` entry point & ISR wrappers)
- [ ] Task Control Blocks (TCB) & Thread Scheduler (Round-robin context switching)

### Phase 3: L4 Primitives & IPC
- [ ] Capability Table & Handle Array management
- [ ] Synchronous Rendezvous IPC (`sys_ipc_send`, `sys_ipc_recv`, `sys_ipc_call`)
- [ ] Userland Page Fault (`#PF`) & Hardware IRQ delegation to IPC

### Phase 4: Userland Bootstrap & Servers
- [ ] Static 64-bit ELF Loader (`PT_LOAD` segment mapper)
- [ ] Spawn Root Task / Pager (Thread 1) into Ring 3
- [ ] Core Userland Servers (`alloc_server`, `vfs_server`, `driver_server`)