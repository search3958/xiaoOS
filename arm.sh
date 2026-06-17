#!/bin/bash
set -e
mkdir -p buildarm/EFI/BOOT
cp limine-bin/BOOTAA64.EFI buildarm/EFI/BOOT/
cp limine.conf buildarm/
echo "Compiling ARM64 kernel (ELF)..."
clang -target aarch64-unknown-none-elf -ffreestanding -fno-stack-protector -fno-stack-check -fno-lto -fno-PIC -mgeneral-regs-only -fshort-wchar -Ikernel -c kernel/main.c -o buildarm/main.o
ld.lld -T kernel/linker_aarch64.lds -o buildarm/kernel buildarm/main.o
echo "Starting QEMU..."
qemu-system-aarch64 \
    -cpu cortex-a57 -M virt \
    -drive if=pflash,format=raw,readonly=on,file=/opt/homebrew/share/qemu/edk2-aarch64-code.fd \
    -drive file=fat:rw:buildarm,format=raw \
    -m 512M -device virtio-gpu-pci -device usb-ehci -device usb-tablet -device usb-kbd -net none
