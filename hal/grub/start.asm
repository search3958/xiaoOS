; Basic assembly entry for Multiboot2
[BITS 32]
section .text
global _start

extern grub_main

_start:
    ; Need to properly define Multiboot2 header here
    call grub_main
    hlt
