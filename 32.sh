#!/bin/bash
set -e
mkdir -p build32/EFI/BOOT
cp limine-bin/BOOTIA32.EFI build32/EFI/BOOT/
cp limine.conf build32/
echo "Compiling x86_32 kernel (ELF)..."
clang -target i386-unknown-none-elf -ffreestanding -fno-stack-protector -fno-stack-check -fno-lto -fno-PIC -m32 -march=i386 -fshort-wchar -Ikernel -c kernel/main.c -o build32/main.o
ld.lld -m elf_i386 -T kernel/linker_i386.lds -o build32/kernel build32/main.o
echo "Starting QEMU..."
qemu-system-i386 \
    -drive if=pflash,format=raw,readonly=on,file=/opt/homebrew/share/qemu/edk2-i386-code.fd \
    -drive file=fat:rw:build32,format=raw \
    -m 512M -vga std -device usb-ehci -device usb-tablet -net none
