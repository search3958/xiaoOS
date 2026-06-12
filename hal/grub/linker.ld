ENTRY(_start)

SECTIONS
{
    . = 1M;

    .multiboot_header : {
        *(.multiboot_header)
    }

    .text : {
        *(.text)
    }

    .rodata : {
        *(.rodata)
    }

    .data : {
        *(.data)
    }

    .bss : {
        bss_start = .;
        *(COMMON)
        *(.bss)
        bss_end = .;
    }
}
