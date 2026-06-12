ENTRY(_start)

SECTIONS
{
    /* Load kernel at 4MB to avoid overlap with GRUB boot info and BIOS/UEFI low memory */
    . = 4M;

    .multiboot_header : ALIGN(4K) {
        KEEP(*(.multiboot_header))
    }

    .text : ALIGN(4K) {
        *(.text .text.*)
    }

    .rodata : ALIGN(4K) {
        *(.rodata .rodata.*)
    }

    .data : ALIGN(4K) {
        *(.data .data.*)
    }

    .bss : ALIGN(4K) {
        bss_start = .;
        *(COMMON)
        *(.bss .bss.*)
        bss_end = .;
    }

    .stack (NOLOAD) : ALIGN(16) {
        stack_bottom = .;
        . += 32K;
        stack_top = .;
    }

    /DISCARD/ : {
        *(.eh_frame)
        *(.note .note.*)
    }
}
