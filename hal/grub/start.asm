bits 32

section .multiboot_header
align 8
multiboot_header_start:
    dd 0xe85250d6                ; magic
    dd 0                         ; architecture (i386)
    dd multiboot_header_end - multiboot_header_start ; length
    dd 0x100000000 - (0xe85250d6 + 0 + (multiboot_header_end - multiboot_header_start)) ; checksum

    ; Framebuffer tag
    align 8
    dw 5                         ; type
    dw 0                         ; flags
    dd 20                        ; size
    dd 1280                      ; width
    dd 720                       ; height
    dd 32                        ; depth

    ; End tag
    align 8
    dw 0                         ; type
    dw 0                         ; flags
    dd 8                         ; size
multiboot_header_end:

section .text
global _start
_start:
    cli
    mov esp, stack_top

    ; Save magic and info
    mov [mb2_magic], eax
    mov [mb2_info], ebx

    ; Zero out .bss (Crucial for Multiboot2)
    extern bss_start
    extern bss_end
    mov edi, bss_start
    xor eax, eax
    mov ecx, bss_end
    sub ecx, edi
    shr ecx, 2
    rep stosd

    ; Check for long mode
    mov eax, 0x80000000
    cpuid
    cmp eax, 0x80000001
    jb no_long_mode
    mov eax, 0x80000001
    cpuid
    test edx, 1 << 29
    jz no_long_mode

    ; Setup paging
    ; Clear page tables
    mov edi, p4_table
    xor eax, eax
    mov ecx, 4096 * (1 + 1 + 4) / 4
    rep stosd

    ; P4[0] -> P3
    mov eax, p3_table
    or eax, 0x03 ; Present | RW
    mov [p4_table], eax

    ; P3[0..3] -> P2 tables (Identity map 4GB)
    mov eax, p2_table
    or eax, 0x03
    mov [p3_table], eax
    add eax, 4096
    mov [p3_table + 8], eax
    add eax, 4096
    mov [p3_table + 16], eax
    add eax, 4096
    mov [p3_table + 24], eax

    ; Fill P2 tables with 2MB pages
    mov ecx, 0
    mov eax, 0x83 ; Present | RW | Huge
.map_p2:
    mov [p2_table + ecx * 8], eax
    add eax, 0x200000
    inc ecx
    cmp ecx, 2048 ; 2048 * 2MB = 4GB
    jne .map_p2

    ; Enable PAE
    mov eax, cr4
    or eax, 1 << 5
    mov cr4, eax

    ; Enable long mode in EFER
    mov ecx, 0xC0000080
    rdmsr
    or eax, 1 << 8
    wrmsr

    ; Enable paging
    mov eax, p4_table
    mov cr3, eax
    mov eax, cr0
    or eax, 1 << 31
    mov cr0, eax

    ; Load GDT
    lgdt [gdt64_ptr]

    ; Jump to 64-bit code
    jmp 0x08:long_mode_start

bits 64
long_mode_start:
    mov ax, 0x10
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax

    ; Arguments for grub_main(magic, addr)
    mov edi, [rel mb2_magic]
    mov esi, [rel mb2_info]

    extern grub_main
    call grub_main

.halt:
    hlt
    jmp .halt

no_long_mode:
    mov al, 'L'
    mov dx, 0x3f8
    out dx, al
    hlt

section .rodata
align 8
gdt64:
    dq 0 ; null
    dq (1 << 43) | (1 << 44) | (1 << 47) | (1 << 53) ; code (64-bit, exec/read)
    dq (1 << 44) | (1 << 47) | (1 << 41)             ; data (64-bit, read/write)
gdt64_end:

gdt64_ptr:
    dw gdt64_end - gdt64 - 1
    dq gdt64

section .bss
align 4096
p4_table:
    resb 4096
p3_table:
    resb 4096
p2_table:
    resb 4096 * 4 ; 4 tables for 4GB

align 16
stack_bottom:
    resb 16384
stack_top:

mb2_magic:
    resd 1
mb2_info:
    resd 1
