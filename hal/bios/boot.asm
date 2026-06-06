%ifndef STAGE2_SECTORS
%define STAGE2_SECTORS 32
%endif

[org 0x7c00]
[bits 16]

start:
    cli
    xor ax, ax
    mov ds, ax
    mov es, ax
    mov ss, ax
    mov sp, 0x7c00
    mov [boot_drive], dl
    sti

    mov ax, 0x0013
    int 0x10

    call set_palette

    call serial_init

    mov dl, [boot_drive]
    mov si, dap
    mov ah, 0x42
    int 0x13
    jc disk_error

    cli
    lgdt [gdt_desc]
    mov eax, cr0
    or eax, 1
    mov cr0, eax
    jmp dword 0x08:0x10000

disk_error:
    mov si, disk_msg
    call serial_print
    mov si, disk_msg
.print:
    lodsb
    test al, al
    jz .hang
    mov ah, 0x0e
    int 0x10
    jmp .print
.hang:
    hlt
    jmp .hang

serial_init:
    mov dx, 0x3f9
    xor al, al
    out dx, al
    mov dx, 0x3fb
    mov al, 0x80
    out dx, al
    mov dx, 0x3f8
    mov al, 0x01
    out dx, al
    mov dx, 0x3f9
    xor al, al
    out dx, al
    mov dx, 0x3fb
    mov al, 0x03
    out dx, al
    mov dx, 0x3fa
    mov al, 0xc7
    out dx, al
    mov dx, 0x3fc
    mov al, 0x0b
    out dx, al
    ret

serial_putc:
    push ax
    push dx
.wait:
    mov dx, 0x3fd
    in al, dx
    test al, 0x20
    jz .wait
    mov dx, 0x3f8
    mov al, bl
    out dx, al
    pop dx
    pop ax
    ret

serial_print:
    lodsb
    test al, al
    jz .done
    mov bl, al
    call serial_putc
    jmp serial_print
.done:
    ret

set_palette:
    push ax
    push bx
    push cx
    push dx

    mov dx, 0x3c8
    xor al, al
    out dx, al
    inc dx

    xor bx, bx
.palette_loop:
    mov al, bl
    shr al, 2
    out dx, al
    out dx, al
    out dx, al
    inc bx
    cmp bx, 256
    jne .palette_loop

    pop dx
    pop cx
    pop bx
    pop ax
    ret

boot_drive: db 0
disk_msg: db "xiaoOS disk read failed", 13, 10, 0

align 4
dap:
    db 0x10
    db 0
    dw STAGE2_SECTORS
    dw 0
    dw 0x1000
    dq 1

gdt_start:
    dq 0
    dq 0x00cf9a000000ffff
    dq 0x00cf92000000ffff
gdt_end:

gdt_desc:
    dw gdt_end - gdt_start - 1
    dd gdt_start

times 510 - ($ - $$) db 0
dw 0xaa55
