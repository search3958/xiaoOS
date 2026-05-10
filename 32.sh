#!/usr/bin/env sh
set -eu

cd "$(dirname "$0")"
make bios

exec qemu-system-i386 \
    -drive file=build/bios/xiao-bios.img,format=raw \
    -nographic \
    -monitor none \
    -no-reboot
