ENTRY(_start32)

SECTIONS {
    . = 0x100000;
    .multiboot_header : { *(.multiboot_header) }
    .text : { *(.text*) }
    .rodata : { *(.rodata*) }
    .data : { *(.data*) }
    .bss : { *(.bss*) *(COMMON) }
}
