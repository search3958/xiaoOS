PHDRS
{
    text    PT_LOAD    FLAGS(5); /* R E */
    rodata  PT_LOAD    FLAGS(4); /* R   */
    data    PT_LOAD    FLAGS(6); /* RW  */
}

SECTIONS
{
    /* We wanna be placed in the higher half, at 0xffffffff80000000. */
    . = 0xffffffff80000000;

    .text : {
        *(.text .text.*)
    } :text

    .rodata : {
        *(.rodata .rodata.*)
    } :rodata

    .data : {
        . = ALIGN(8);
        KEEP(*(.limine_requests))
        *(.data .data.*)
    } :data

    .bss : {
        *(.bss .bss.*)
        *(COMMON)
    } :data

    /DISCARD/ : {
        *(.eh_frame)
        *(.note .note.*)
    }
}
