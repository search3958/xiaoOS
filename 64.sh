#!/bin/bash
set -e
mkdir -p build64/EFI/BOOT
cp limine-bin/BOOTX64.EFI build64/EFI/BOOT/
cp limine.conf build64/
echo "Compiling x86_64 kernel..."
clang -target x86_64-unknown-windows -fuse-ld=lld -ffreestanding -fno-stack-protector -fshort-wchar -mno-red-zone -nostdlib -Wl,-entry:EfiMain -Wl,-subsystem:efi_application -Ikernel -o build64/kernel.efi kernel/main.c
echo "Starting QEMU..."
qemu-system-x86_64 -bios /opt/homebrew/share/qemu/edk2-x86_64-code.fd -drive file=fat:rw:build64,format=raw -m 512M -net none
