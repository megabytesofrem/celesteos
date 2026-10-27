[bits 64]
section .text

%include "asm_macros.inc"

global spinlock_init
global spinlock_try_acquire
global spinlock_acquire
global spinlock_release
global spinlock_acquire_irqsave
global spinlock_release_irqrestore

; ---------------------------------------------------
; void spinlock_init(spinlock_t *lock [RDI])
; ---------------------------------------------------
; 
; Initialize the spinlock to an unlocked state
spinlock_init:
    mov dword [rdi], 0
    ret

; ---------------------------------------------------
; void spinlock_acquire(spinlock_t *lock [RDI])
; ---------------------------------------------------
; 
; Acquire the spinlock, spinning until it becomes available
spinlock_acquire:
.spin:
    lock bts dword [rdi], 0     ; Atomically test bit 0 of the lock and set it to 1
    jnc .acquired               ; Jump if the lock was successfully acquired (carry flag not set)
.wait:
    pause                       ; Hint to the processor that we are in a spin-wait loop 
    jmp .spin                   ; Retry acquiring the lock
.acquired:
    ret                         ; We have successfully acquired the lock, return to the caller


; ---------------------------------------------------
; int spinlock_try_acquire(spinlock_t *lock [RDI])
; ---------------------------------------------------
; 
; Try to acquire the spinlock, return 1 if successful, 0 otherwise
spinlock_try_acquire:
    mov eax, 1
    lock xchg eax, dword [rdi]   ; Atomically exchange the value of the lock with the value in EAX
    xor eax, 1                   ; Set EAX to 1 if the lock was successfully acquired, 0 otherwise
    ret                          ; Return 1 if the lock was successfully acquired, 0 otherwise


; ---------------------------------------------------
; void spinlock_release(spinlock_t *lock [RDI])
; ---------------------------------------------------
; 
; Release the spinlock, making it available for other threads to acquire
spinlock_release:
    lock btr dword [rdi], 0     ; Atomically clear bit 0 of the lock
    ret                         ; Return to the caller

; ---------------------------------------------------
; void spinlock_acquire_irqsave(spinlock_t *lock [RDI])
; ---------------------------------------------------
; 
; Acquire the spinlock and disable local interrupts, saving the current interrupt state
spinlock_acquire_irqsave:
    pushfq                      ; Save the current flags on the stack (RFLAGS)
    pop rax                     ; Pop the saved flags into RAX
    cli                         ; Disable local interrupts

    call spinlock_acquire       ; Call the regular spinlock acquire function
    ret                         ; Return to the caller with interrupts disabled and the spinlock acquired

; -------------------------------------------------------------------------------
; void spinlock_release_irqrestore(spinlock_t *lock [RDI], uint64_t flags [RSI])
; -------------------------------------------------------------------------------
; 
; Release the spinlock and restore the saved interrupt state
spinlock_release_irqrestore:
    ; Release the lock
    lock btr dword [rdi], 0     ; Atomically clear bit 0 of the lock

    ; Check if bit 9 was set
    bt rsi, 9                   ; Test bit 9 of the saved flags (RFLAGS)
    IF c                        ; If carry flag is set (bit 9 of RFLAGS was set)
        sti                     ; If bit 9 was set, re-enable local interrupts
    ENDIF
    ret                         ; Return to the caller