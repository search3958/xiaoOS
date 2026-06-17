#!/bin/bash
set -e
mkdir -p build32/EFI/BOOT
cp limine-bin/BOOTIA32.EFI build32/EFI/BOOT/
cp limine.conf build32/
echo "Compiling x86_32 kernel..."
clang -target i386-unknown-windows -fuse-ld=lld -ffreestanding -fno-stack-protector -fshort-wchar -nostdlib -Wl,-entry:EfiMain -Wl,-subsystem:efi_application -Ikernel -o build32/kernel.efi kernel/main.c
echo "Starting QEMU..."
qemu-system-i386 -bios /opt/homebrew/share/qemu/edk2-i386-code.fd -drive file=fat:rw:build32,format=raw -m 512M -net none
