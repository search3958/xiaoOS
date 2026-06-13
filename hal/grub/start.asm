; Multiboot2 header
section .text
align 8
header_start:
    dd 0xE85250D6                ; magic
    dd 0                         ; architecture 0 (i386)
    dd header_end - header_start ; header length
    dd 0x100000000 - (0xE85250D6 + 0 + (header_end - header_start)) ; checksum
    
    ; Address tag
    dw 2
    dw 0
    dd 24
    dd header_start
    dd _start32
    dd 0
    dd 0
    
    ; Entry point tag
    dw 3
    dw 0
    dd 12
    dd _start32
    
    ; End tag
    dw 0
    dw 0
    dd 8
header_end:

[BITS 32]
global _start32
extern grub_main

_start32:
    ; Set up stack
    mov esp, stack_top
    call grub_main
    cli
.halt:
    hlt
    jmp .halt

section .bss
align 16
stack_bottom:
    resb 16384
stack_top:
