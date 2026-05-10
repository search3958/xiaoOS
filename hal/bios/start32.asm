[bits 32]

global _start32
extern xiao_bios_main

_start32:
    mov ax, 0x10
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax
    mov esp, 0x90000
    call xiao_bios_main
.hang:
    hlt
    jmp .hang
