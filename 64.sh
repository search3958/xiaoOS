#!/usr/bin/env sh
set -eu
cd "$(dirname "$0")"

make grub

# Update ISO
mkdir -p build/iso/boot/grub
cp build/grub/xiaoos.bin build/iso/boot/
cp hal/grub/grub.cfg build/iso/boot/grub/
/opt/homebrew/bin/i686-elf-grub-mkrescue -o build/xiaoos.iso build/iso

exec qemu-system-x86_64 \
    -machine q35 \
    -accel tcg \
    -m 512M \
    -cdrom build/xiaoos.iso \
    -monitor none \
    -no-reboot
