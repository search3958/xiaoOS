#!/usr/bin/env sh
set -eu

cd "$(dirname "$0")"
make bios

exec qemu-system-i386 \
    -m 512M \
    -drive file=build/bios/xiao-bios.img,format=raw \
    -serial stdio \
    -monitor none \
    -no-reboot
