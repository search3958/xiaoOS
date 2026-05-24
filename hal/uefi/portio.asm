[bits 64]

global uefi_outb
global uefi_inb

section .text

uefi_outb:
    mov al, dl
    mov dx, cx
    out dx, al
    ret

uefi_inb:
    mov dx, cx
    in al, dx
    ret
