## Currently Implemented
- [x] CPU IDT/GDT/PIC
- [x] CPU interrupt handling (ISR/IRQs)
- [x] Framebuffer driver
- [x] Physical and virtual memory manager (`pmap.c`)
- [x] `kmalloc`/`kfree` functions
- [x] Spinlock and TSS (Task State Segment)

## Useful Links
- **Apple official kernel documentation**: https://developer.apple.com/documentation/kernel/
- **Apple XNU source code**: https://github.com/apple-oss-distributions/xnu/blob/main/osfmk/
- **CMU Mach 3.0 kernel**: https://github.com/Prajna/mach/tree/master/kernel/mach
- **Darling Project**: https://github.com/darlinghq/darling

## Code Hygiene
- `vmm.h` was renamed to `pmap.h` to mirror Mach more accurately, as it was a weird acronym because
the virtual memory manager does not really manage anything; it maps virtual addresses to physical addresses using page tables (PML4/PDPT/PD/PT).

- `kern_return_t` is used everywhere to indicate success or failure (and *why*), typically returning `KERN_SUCCESS` on success and an appropriate error code on failure. It is absent from the lowest level code e.g PIC, GDT, IDT.

If in doubt, use `kern_return` everywhere in the `mach/` subdirectory, and sparsely everywhere else. If an int or void suffices, that is what should be used (e.g., `heap_initialize` returns an int instead of `kern_return_t`), `vm_map` is __an exception__.

- **General advice**: If in doubt, we mirror Mach/XNU conventions as we are aiming for binary compatibility ala ReactOS.

## Priority
- Mach loader
- Mach IPC logic (`mach_message.c`, `port_space.c`, `thread.c`, `task.c`, etc)
- APIC (Advanced Programmable Interrupt Controller)
- CPUID (CPU Identification)
- HPET (High Precision Event Timer)

## In Progress
- **Mach loader**: Currently can parse Mach-O binaries, extract their load commands and map them into memory.
Partially implemented, but no support for dylinkers, relocation, dynamic symbols, etc.
- **Mach IPC**: Currently a scaffold of headers and structures.

## Pipedreams for now
- Reimplementation of Apple's `libSystem` and CRT
- Reimplementation of Apple's `launchd` (PID 1 init)
- Reimplementation of Apple's `libdispatch` (Grand Central Dispatch)
- Reimplementation of Apple's `dyld`
- Reimplementation of Apple's `CoreFoundation`
- Implement a display server based on Wayland

### Mach IPC
- `mach_message.h`: This defines the structure of a Mach IPC message, and provides functions for creating, sending, and receiving messages.
- `port.h`: This defines the structure and management of Mach ports, which are used for inter-process communication and represent communication endpoints within the Mach IPC system.
- `port_space.h`: This defines the structure and management of Mach port spaces, which are used to keep track of the ports owned by a task and their associated rights.
- `thread.h`: This defines the structure and management of Mach threads, which represent the execution context of a single thread of execution within a task. Not to be confused with Posix threads.
- `task.h`: This defines the structure and management of Mach tasks, which represent the execution context of a process and its associated resources (i.e., memory, threads, and port rights).