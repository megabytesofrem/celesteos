; -----------------------------------------------------------------------------------
; pushall / popall - Macros to save and restore all general purpose registers
; -----------------------------------------------------------------------------------
%macro pushall 0
    push rax
    push rbx
    push rcx
    push rdx
    push rsi
    push rdi
    push rbp
    push r8
    push r9
    push r10
    push r11
    push r12
    push r13
    push r14
    push r15
%endmacro

%macro popall 0
    pop r15
    pop r14
    pop r13
    pop r12
    pop r11
    pop r10
    pop r9
    pop r8
    pop rbp
    pop rdi
    pop rsi
    pop rdx
    pop rcx
    pop rbx
    pop rax
%endmacro

; -----------------------------------------------------------------------------------
; IF / ELSE / ENDIF macros for conditional assembly
;
; Usage:
;   IF <value1>, <operator>, <value2>
;       ; code to execute if condition is true
;   ELSE
;       ; code to execute if condition is false
;   ENDIF
; -----------------------------------------------------------------------------------
%macro IF 3
    %push if_stack                  ; push a new context onto the macro stack
    cmp %1, %3                      ; compare the two values for the IF condition
    j%-2 ..@else_label              ; jump to ELSE label if IF condition is false
%endmacro

%macro ELSE 0
    %ifctx if_stack
        jmp ..@end_if_label         ; jump over the ELSE block when IF condition was true
        ..@else_label:              ; target for the initial failed IF jump
        %repl if_else_stack         ; convert stack context from IF to IF_ELSE
    %else
        %error "ELSE used without a matching IF"
    %endif
%endmacro

%macro ENDIF 0
    %ifctx if_stack
        ..@end_if_label:              ; target for the end of the IF/ELSE block
        %pop                          ; pop the IF context from the macro stack
    %elifctx if_else_stack
        ..@end_if_label:              ; target for the end of the IF/ELSE block
        %pop                          ; pop the IF_ELSE context from the macro stack
    %else
        %error "ENDIF used without a matching IF"
    %endif
%endmacro

; -----------------------------------------------------------------------------------
; FOR / ENDFOR macros for loop assembly
;
; Usage:
;   FOR <loop_var>, <start_value>, <end_value>
;       ; code to execute for each iteration
;   ENDFOR
; -----------------------------------------------------------------------------------
%macro FOR 3
    %push for_stack                  ; push a new context onto the macro stack
    mov %1, %2                       ; initialize the loop variable
    ..@for_label:                    ; start of the FOR loop
    cmp %1, %3                       ; compare the loop variable with the end value
    j%-2 ..@end_for_label            ; jump to end label if FOR condition is false
%endmacro

%macro ENDFOR 0
    %ifctx for_stack
        jmp ..@for_label                 ; jump back to the start of the FOR loop
        ..@end_for_label:               ; target for the end of the FOR loop
        %pop                           ; pop the FOR context from the macro stack
    %else
        %error "ENDFOR used without a matching FOR"
    %endif
%endmacro

; -----------------------------------------------------------------------------------
; WHILE / ENDWHILE macros for loop assembly
;
; Usage:
;   WHILE <value1>, <value2>
;       ; code to execute while condition is true
;   ENDWHILE
; -----------------------------------------------------------------------------------
%macro WHILE 2
    %push while_stack                  ; push a new context onto the macro stack
    cmp %1, %2                         ; compare the two values for the WHILE condition
    j%-2 ..@end_while_label            ; jump to end label if WHILE condition is false
%endmacro

%macro ENDWHILE 0
    %ifctx while_stack
        jmp ..@while_label             ; jump back to the start of the WHILE loop
        ..@end_while_label:             ; target for the end of the WHILE loop
        %pop                            ; pop the WHILE context from the macro stack
    %else
        %error "ENDWHILE used without a matching WHILE"
    %endif
%endmacro