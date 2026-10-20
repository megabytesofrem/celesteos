# AGENTS.md

## Core Architectural Mission

celesteOS is an open-source operating system designed from scratch to achieve **binary compatibility with Mac OS X 10.6 Snow Leopard at the kernel level**. 

celesteOS is a **Darwin-like operating system** that functions as a strict **clean-room implementation of the Darwin kernel**. The ultimate goal is to execute 64-bit Mach-O binaries cleanly on bare-metal hardware via standard Limine boot hand-offs.

---

## Strict Clean-Room & Legal Rules

All AI collaborators, autonomous agents, and human contributors working on celesteOS **MUST** follow these mandatory rules:

1. **PROHIBITED: Direct XNU Source Code Usage**
   * Agents are **strictly forbidden** from copying, translating, importing, or adapting code directly from Apple’s XNU source releases or any APSL-licensed (Apple Public Source License) repository.
   * Implementation logic (`.c`, `.cpp`, or assembly function bodies) from Apple's XNU tree must **NEVER** enter the celesteOS repository.

2. **PERMITTED: Header-Only Reference**
   * Apple headers (`.h` files), public Mach 3.0 specification declarations, structure offsets, negative Mach trap indices (`-28`), and BSD syscall table offsets (`0x2000000`) may be used **EXCLUSIVELY as interface and specification references**.
   * Headers define the *what* (the ABI contract); celesteOS writes 100% of the *how* (the Ring 0 execution logic).

3. **Binary Compatibility ≠ 1:1 Cloning**
   * celesteOS aims for **functional ABI contract fulfillment**, NOT a 1:1 pixel-for-pixel or code-for-code clone of the macOS platform stack.
   * **Kernel Space:** Satisfy the Mach trap and BSD syscall table interface directly on bare-metal hardware.
   * **Display & Graphics:** Do not attempt to clone `IOKit` accelerators, `CoreGraphics` server internals, or `WindowServer`. Decouple display rendering by routing framebuffers and window surfaces to open-source protocols like **Wayland or X11**.
   * **Frameworks & Framework Shims:** High-level APIs (`AppKit`, `Foundation`) and system daemons (`launchd`) are fulfilled using clean-room userland shims (e.g., Cocotron, GNUstep, custom C runtime wrappers), not proprietary Apple implementations.

---

## Technical Stack & Reference Boundaries

| Component | celesteOS Strategy | Permitted Reference |
| :--- | :--- | :--- |
| **Boot System** | Bare-metal hand-off using **Limine** loader | Standard Limine protocol specs |
| **Kernel Engine** | Custom C/Assembly Ring 0 trap dispatcher & memory manager | Public Mach 3.0 specs, VM symbol dumps |
| **Syscall ABI** | Native `-28` Mach traps & `0x2000000` BSD syscall offsets | Live 10.6 VM trap tables & public header constants |
| **Userland Glue** | Custom `libSystem.dylib` shim backed by ported BSD tools | Public C standard library & POSIX headers |

---

## Developer & Agent Directive

When generating code, proposed architectures, or file layouts for celesteOS:

* **Always** write original, clean C and x86_64 Assembly implementation bodies.
* **Always** design modular, independent subsystems (memory allocators, IDT vector tables, IPC port structures) rather than mirroring XNU's internal driver structures.
* **Never** suggest importing APSL-licensed files or Apple framework binaries. Focus entirely on satisfying the binary execution contract at the interface boundary.