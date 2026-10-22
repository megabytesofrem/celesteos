## Currently Implemented
- [x] CPU IDT/GDT/PIC
- [x] CPU interrupt handling (ISR/IRQs)
- [x] Framebuffer driver
- [x] Physical and virtual memory manager (`pmap.c`)
- [x] `kmalloc`/`kfree` functions
- [x] Spinlock and TSS (Task State Segment)
- [x] CPUID (CPU Identification)
- [x] Mach/BSD syscall stubs
- [x] Wired up basic IPC for the kernel (ports, message passing)

## Useful Links
- **Apple official kernel documentation**: https://developer.apple.com/documentation/kernel/
- **Apple XNU source code (Snow Leopard)**: https://github.com/apple-oss-distributions/xnu/tree/rel/xnu-1504
- **CMU Mach 3.0 kernel**: https://github.com/Prajna/mach/tree/master/kernel/mach
- **Darling Project**: https://github.com/darlinghq/darling

## Initrd Notes
The kernel mounts an initramfs (initrd) at boot which replicates a native Mac OS X filesystem structure.
```
userspace/rootfs/ (template; generated output is written under build/rootfs/)
├── Applications
├── Library
│   ├── Application Support
│   ├── LaunchAgents
│   ├── LaunchDaemons
│   └── Preferences
│       └── SystemConfiguration
│           └── com.celesteOS.Boot.plist
├── private
│   ├── etc
│   ├── tmp
│   └── var
├── System
│   └── Library
│       ├── CoreServices
│       ├── Extensions
│       ├── Frameworks
│       ├── LaunchAgents
│       └── LaunchDaemons
├── usr
│   ├── bin
│   │   ├── bsd_trap
│   │   └── stub.c
│   ├── lib
│   └── libexec
│       ├── launchd
│       └── launchd.c
└── Volumes
```

`launchd` is PID 1, and is the first user-space process started by the kernel, responsible for launching and managing other system services and daemons.

`/Library/Preferences` contains user and system preference files, including `SystemConfiguration/com.celesteOS.Boot.plist` which holds boot configuration settings, used by celesteOS later on.


## Dumping Mach Traps
Scripts are provided in `scripts/osx/` to dump kernel traps from a running `/mach_kernel`, as well to parse said traps into a tabular format.  

You will need a real OSX 10.6 system to run them, a virtual machine can be setup if necessary.

Other useful commands are:

### Locating mach_trap_table symbol:
`nm /mach_kernel | grep -i mach_trap_table`

### Inspecting segment headers:
`otool -l /mach_kernel`

### Inspecting binaries:
- `nm -g /usr/lib/libSystem.B.dylib | grep -e "mach_|semaphore_|task_"` can be used to list all symbols related to Mach IPC, tasks, and threads within the Mach subsystem.

- `otool -L /usr/lib/libSystem.B.dylib` can be used to list all shared libraries that `libSystem.B.dylib` depends on.

- `otool -tv /usr/lib/libSystem.B.dylib` can be used to display the disassembled text (code) section of `libSystem.B.dylib` (the output may be large, so it should be paged or redirected to a file).

## Syscalls
- Mach syscalls are negative (starting at -26, `mach_reply_port`)
- BSD syscalls are offset by `0x2000000` (e.g., `sys_write` is `0x2000004`), rather than `4` as in traditional Unix-like systems.

Syscall number is held in `rax`, with arguments passed in the following registers:
- `rdi`: first argument
- `rsi`: second argument
- `rdx`: third argument
- `r10`: fourth argument
- `r8`: fifth argument
- `r9`: sixth argument

`r11` and `rcx` hold usermode RFLAGS and RIP (the return address) temporarily during the syscall.

A list of all the syscalls are defined at: [xnu syscall_sw.h](https://github.com/apple-oss-distributions/xnu/blob/rel/xnu-1504/osfmk/mach/i386/syscall_sw.h)

And for the BSD syscalls:
- [FreeBSD syscalls table](https://alfonsosiciliano.gitlab.io/posts/2021-01-02-freebsd-system-calls-table.html)
- [OpenBSD syscalls.master](https://github.com/openbsd/src/blob/master/sys/kern/syscalls.master)

Syscalls are defined once in `kernel/syscalls.master`, and are built by the build system into the appropriate syscall tables and stubs.


## Priority
- Mach loader
- Mach IPC logic (`mach_message.c`, `port_space.c`, `thread.c`, `task.c`, etc)
- Stub out syscalls (`sys_task_self_trap`, `sys_mach_reply_port`, etc)
- APIC (Advanced Programmable Interrupt Controller)
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