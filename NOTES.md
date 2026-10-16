## Currently Implemented
- [x] CPU IDT/GDT/PIC
- [x] CPU interrupt handling (ISR/IRQs)
- [x] Framebuffer driver
- [x] Physical and virtual memory manager 
- [x] `kmalloc`/`kfree` functions
- [x] Spinlock and TSS (Task State Segment)

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