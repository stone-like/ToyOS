section .asm

extern int21h_handler
extern no_interrupt_handler

global idt_load
global int21h
global no_interrupt

idt_load:
    push ebp
    mov ebp, esp

    mov ebx, [ebp+8] ; +8が第一引数、+4がretutnアドレス
    lidt [ebx]
    pop ebp
    ret

int21h:
    cli
    pushad
    call int21h_handler
    popad
    sti
    iret

no_interrupt:
    cli
    pushad
    call no_interrupt_handler
    popad
    sti
    iret