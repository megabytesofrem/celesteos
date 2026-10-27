[bits 64]
section .text

%include "asm_macros.inc"

global spinlock_init
global spinlock_try_acquire
global spinlock_acquire
global spinlock_release
global spinlock_acquire_irqsave
global spinlock_release_irqrestore

; ------------------------------------------------------------------------------
; spinlock_init - Initialize the spinlock to an unlocked state
; 
; Arguments:
;   RDI - pointer to the spinlock structure
; ------------------------------------------------------------------------------
spinlock_init:
    mov dword [rdi], 0
    ret

; ------------------------------------------------------------------------------
; spinlock_acquire - Acquire the spinlock, spinning until it becomes available
;
; Arguments:
;   RDI - pointer to the spinlock structure
; ------------------------------------------------------------------------------
spinlock_acquire:
.spin:
    lock bts dword [rdi], 0     ; atomically test bit 0 of the lock and set it to 1
    jnc .acquired               ; jump if the lock was successfully acquired (carry flag not set)
.wait:
    pause                       ; hint to the processor that we are in a spin-wait loop 
    jmp .spin                   ; retry acquiring the lock
.acquired:
    ret                         ; we have successfully acquired the lock, return to the caller


; ------------------------------------------------------------------------------
; spinlock_try_acquire - Try to acquire the spinlock
;
; Arguments:
;   RDI - pointer to the spinlock structure
; Returns:
;   EAX - EAX contains 1 if the lock was successfully acquired, 0 otherwise
; ------------------------------------------------------------------------------
spinlock_try_acquire:
    mov eax, 1
    xchg eax, dword [rdi]        ; memory XCHG is implicitly atomic, no lock needed
    xor eax, 1                   ; set EAX to 1 if the lock was successfully acquired, 0 otherwise
    ret                          ; return 1 if the lock was successfully acquired, 0 otherwise


; ------------------------------------------------------------------------------
; spinlock_release - Release the spinlock, making it available for other threads to 
; acquire the lock
; 
; Arguments:
;   RDI - pointer to the spinlock structure
; ------------------------------------------------------------------------------
; 
; Release the spinlock, making it available for other threads to acquire
spinlock_release:
    lock btr dword [rdi], 0     ; atomically clear bit 0 of the lock
    ret                         ; return to the caller

; ------------------------------------------------------------------------------
; spinlock_acquire_irqsave - Acquire the spinlock and disable local interrupts, 
; saving the current interrupt state
;
; Arguments:
;   RDI - pointer to the spinlock structure
; Returns:
;   RAX - saved interrupt flags (RFLAGS)
; ------------------------------------------------------------------------------
spinlock_acquire_irqsave:
    pushfq                      ; save the current flags on the stack (RFLAGS)
    pop rax                     ; pop the saved flags into RAX
    cli                         ; disable local interrupts

    call spinlock_acquire       ; call the regular spinlock acquire function
    ret                         ; return to the caller with interrupts disabled and the spinlock acquired

; -------------------------------------------------------------------------------
; spinlock_release_irqrestore - Release the spinlock and restore the saved interrupt state
;
; Arguments:
;   RDI - pointer to the spinlock structure
;   RSI - saved interrupt flags (RFLAGS)
; -------------------------------------------------------------------------------
spinlock_release_irqrestore:
    ; Release the lock
    lock btr dword [rdi], 0     ; atomically clear bit 0 of the lock

    ; Check if bit 9 was set
    bt rsi, 9                   ; test bit 9 of the saved flags (RFLAGS)
    jnc .interrupts_restored    ; if bit 9 was clear, leave interrupts disabled
    sti                         ; otherwise, re-enable local interrupts
.interrupts_restored:
    ret                         ; return to the caller