# AI Coding Assistant Guidelines for CelesteOS

## Critical Architecture Boundaries
- **This is NOT Windows NT, Windows, or ReactOS.** CelesteOS is an independent, custom-built microkernel utilizing an Object Executive architecture.
- **DO NOT** suggest, reference, or replicate source code, data structures, headers, or algorithms from the ReactOS project or Windows NT internals.
- **DO NOT** assume standard Windows kernel APIs, types (e.g., `PVOID`, `NTSTATUS`, `PEPROCESS`), or private header files exist here.

---

## Architectural & Design Patterns
- **Object Executive Model:** Subsystems follow explicit modular layouts (`ke/` for kernel primitives, `ob/` for the object manager, `ps/` for process/thread management).
- **Handle Safety:** Handle tables use generation-tagging and explicit bitmask access checks. Always increment generation counters on handle destruction to prevent reuse vulnerabilities.
- **Reference Counting:** Strict manual lifecycle management via `ObRetain()` and `ObRelease()`. Every handle acquisition must balance its reference count.
- **Concurrency:** Fine-grained locking via `KeSpinLock` and atomic operations (`stdatomic.h`, memory order constraints) rather than heavy global locks.

---

## Naming Conventions & Style
- **Subsystem Prefixes:** Use consistent subsystem prefixes for functions and types (`Ps` for processes/tasks, `Ob` for objects, `Ke` for kernel primitives).
- **Clarity Over Bloat:** Keep implementations lean, minimalist, and mathematically sound. Avoid enterprise-grade backwards-compatibility cruft.