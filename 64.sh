#!/bin/bash
set -e
mkdir -p build64/EFI/BOOT
cp limine-bin/BOOTX64.EFI build64/EFI/BOOT/
cp limine.conf build64/
echo "Compiling x86_64 kernel (ELF)..."
clang -target x86_64-unknown-none-elf -ffreestanding -fno-stack-protector -fno-stack-check -fno-lto -fno-PIC -m64 -march=x86-64 -mno-80387 -mno-mmx -mno-sse -mno-sse2 -mno-red-zone -mcmodel=kernel -fshort-wchar -Ikernel -c kernel/main.c -o build64/main.o
ld.lld -T kernel/linker.lds -o build64/kernel build64/main.o
echo "Starting QEMU..."
qemu-system-x86_64 \
    -drive if=pflash,format=raw,readonly=on,file=/opt/homebrew/share/qemu/edk2-x86_64-code.fd \
    -drive file=fat:rw:build64,format=raw \
    -m 512M -vga std -device usb-ehci -device usb-tablet -net none
