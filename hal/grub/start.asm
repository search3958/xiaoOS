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

section .data
align 16
mb2_magic_save: dd 0
mb2_info_save:  dd 0

; Page tables in .data
align 4096
p4_table: resb 4096
p3_table: resb 4096
p2_table: resb 4096 * 4

section .text
global _start
_start:
    cli
    mov [mb2_magic_save], eax
    mov [mb2_info_save], ebx

    extern stack_top
    mov esp, stack_top

    ; Enable SSE/FPU (Required for x86_64 floats)
    mov eax, cr0
    and ax, 0xFFFB      ; Clear EM (bit 2)
    or ax, 0x0002       ; Set MP (bit 1)
    mov cr0, eax
    mov eax, cr4
    or ax, 3 << 9       ; Set OSFXSR (bit 9) and OSXMMEXCPT (bit 10)
    mov cr4, eax

    ; Zero out .bss
    extern bss_start
    extern bss_end
    mov edi, bss_start
    xor eax, eax
    mov ecx, bss_end
    sub ecx, edi
    shr ecx, 2
    rep stosd

    ; Setup paging (Identity map 4GB)
    mov edi, p4_table
    xor eax, eax
    mov ecx, 4096 * 6 / 4
    rep stosd

    mov eax, p3_table
    or eax, 0x03 ; Present | RW
    mov [p4_table], eax

    mov eax, p2_table
    or eax, 0x03 ; Present | RW
    mov [p3_table], eax
    add eax, 4096
    mov [p3_table + 8], eax
    add eax, 4096
    mov [p3_table + 16], eax
    add eax, 4096
    mov [p3_table + 24], eax

    mov ecx, 0
    mov eax, 0x83 ; Present | RW | Huge
.map_p2:
    mov [p2_table + ecx * 8], eax
    add eax, 0x200000
    inc ecx
    cmp ecx, 2048 ; 4GB
    jne .map_p2

    ; Enable PAE
    mov eax, cr4
    or eax, 1 << 5
    mov cr4, eax

    ; Enable long mode
    mov ecx, 0xC0000080
    rdmsr
    or eax, 1 << 8 ; LME
    wrmsr

    ; Enable paging
    mov eax, p4_table
    mov cr3, eax
    mov eax, cr0
    or eax, 1 << 31 ; PG
    mov cr0, eax

    lgdt [gdt64_ptr]
    jmp 0x08:long_mode_start

bits 64
long_mode_start:
    mov ax, 0x10
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax

    and rsp, -16
    sub rsp, 8

    mov edi, [rel mb2_magic_save]
    mov esi, [rel mb2_info_save]

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
    dq (1 << 43) | (1 << 44) | (1 << 47) | (1 << 53) ; code
    dq (1 << 44) | (1 << 47) | (1 << 41)             ; data
gdt64_end:

gdt64_ptr:
    dw gdt64_end - gdt64 - 1
    dq gdt64
