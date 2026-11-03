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
; IF / ELSE / ENDIF macros for runtime conditional branches
;
; Usage:
;   IF <condition>
;       ; code executed when the condition is true
;   ELSE
;       ; code executed when the condition is false
;   ENDIF
; Or:
;   IF <value1>, <condition>, <value2>
;       ; code executed when value1 <condition> value2
;   ENDIF
; -----------------------------------------------------------------------------------
%macro IF 1-3
    %push if_stack

    %if %0 == 1
        j%-1 %$if_body
    %elif %0 == 3
        cmp %1, %3
        j%-2 %$if_body
    %else
        %error "IF macro requires either 1 argument (flag) or 3 arguments (val1, op, val2)"
    %endif

    jmp %$else_label
    %$if_body:
%endmacro

%macro ELSE 0
    %ifctx if_stack
        jmp %$end_if
        %$else_label:
        %repl if_else_stack
    %else
        %error "ELSE used without a matching IF"
    %endif
%endmacro

%macro ENDIF 0
    %ifctx if_stack
        %$else_label:
        %pop
    %elifctx if_else_stack
        %$end_if:
        %pop
    %else
        %error "ENDIF used without a matching IF"
    %endif
%endmacro

; -----------------------------------------------------------------------------------
; FOR / ENDFOR macros for loop assembly
;
; Usage:
;   FOR <loop_var>, <start_value>, <end_value>
;       ; code executed for each value, inclusive
;   ENDFOR
; -----------------------------------------------------------------------------------
%macro FOR 3
    %push for_stack
    %define %$for_var %1
    mov %1, %2
    %$for_begin:
    cmp %1, %3
    jg %$for_end
%endmacro

%macro ENDFOR 0
    %ifctx for_stack
        inc %$for_var
        jmp %$for_begin
        %$for_end:
        %pop
    %else
        %error "ENDFOR used without a matching FOR"
    %endif
%endmacro

; -----------------------------------------------------------------------------------
; WHILE / ENDWHILE macros for loop assembly
;
; Usage:
;   WHILE <value1>, <condition>, <value2>
;       ; code executed while value1 <condition> value2 is true
;   ENDWHILE
; -----------------------------------------------------------------------------------
%macro WHILE 3
    %push while_stack
    %$while_begin:
    cmp %1, %3
    j%-2 %$while_body
    jmp %$while_end
    %$while_body:
%endmacro

%macro ENDWHILE 0
    %ifctx while_stack
        jmp %$while_begin
        %$while_end:
        %pop
    %else
        %error "ENDWHILE used without a matching WHILE"
    %endif
%endmacro