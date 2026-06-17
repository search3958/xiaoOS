#!/bin/bash
set -e
mkdir -p buildarm/EFI/BOOT
cp limine-bin/BOOTAA64.EFI buildarm/EFI/BOOT/
cp limine.conf buildarm/
cp limine.conf buildarm/EFI/BOOT/
echo "Compiling ARM64 kernel..."
clang -target aarch64-unknown-windows -fuse-ld=lld -ffreestanding -fno-stack-protector -fshort-wchar -nostdlib -Wl,-entry:EfiMain -Wl,-subsystem:efi_application -Ikernel -o buildarm/kernel.efi kernel/main.c
echo "Starting QEMU..."
qemu-system-aarch64 \
    -cpu cortex-a57 -M virt \
    -drive if=pflash,format=raw,readonly=on,file=/opt/homebrew/share/qemu/edk2-aarch64-code.fd \
    -drive file=fat:rw:buildarm,format=raw \
    -m 512M \
    -device virtio-gpu-pci \
    -device usb-ehci -device usb-mouse -device usb-kbd \
    -net none
