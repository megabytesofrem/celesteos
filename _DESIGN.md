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