## Code Hygiene
- `vmm.h` was renamed to `pmap.h` to mirror Mach more accurately, as it was a weird acronym because
the virtual memory manager does not really manage anything; it maps virtual addresses to physical addresses using page tables (PML4/PDPT/PD/PT).

- `kern_return_t` is used everywhere to indicate success or failure (and *why*), typically returning `KERN_SUCCESS` on success and an appropriate error code on failure. It is absent from the lowest level code e.g PIC, GDT, IDT.

- `kern_return_t` is also absent from a majority of low level IPC functions, as they are strictly internal and do not need to communicate success or failure to user-space.

- **General advice**: If in doubt, we mirror Mach/XNU conventions as we are aiming for binary compatibility ala ReactOS.

## Kernel IPC vs User Mach
- **Kernel IPC**: Internal IPC routines and handlers defined in `kernel/ipc` that manage Mach ports, IPC spaces,
kmsgs via Mach messages, etc.

- **User Mach**: The user-space Mach IPC routines and libraries that interact with the kernel IPC mechanisms, typically through system calls and Mach messages (defined in `kernel/mach`). This is the interface that user-space programs use to communicate with the kernel's Mach IPC system.

## Kernel IPC objects
`mach_port_t` is an alias for the kernel's `ipc_port_t`, whose first member is
`ipc_object_t`. The common header owns the port's lock, reference count, active
bit, and object/kernel-object type bits. IPC-space table entries refer to this
header; messages are not IPC objects.

Tasks, threads, and IPC spaces retain their own locks; they are not IPC objects.
Port sets are not implemented yet.

Use the object lock when accessing shared object state. The `io_reference` and
`io_release` macros require that lock; `ipc_object_reference` and
`ipc_object_release` acquire it internally. Callers must own a live reference
before retaining or accessing an object and detach type-specific resources
(including queued messages) before its final release.

IPC-space insertion/lookup, reference-counted port allocation, and per-task port names are
implemented. User traps can allocate receive rights, create a separate send name, deallocate a
name, and send/receive simple messages through `mach_msg`. User buffers are copied through the
active page tables; complex messages and descriptor/rights transfer are not implemented. Receive
is currently nonblocking: an empty queue returns immediately because the kernel has no context
switch scheduler yet. Task suspend/resume and actual thread scheduling remain unavailable.