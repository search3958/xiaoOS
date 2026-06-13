#!/usr/bin/env sh
set -eu

cd "$(dirname "$0")"

make grub

detect_default_accel() {
    os="$(uname -s)"
    arch="$(uname -m)"

    case "$os:$arch" in
        Darwin:x86_64)
            printf 'hvf\n'
            ;;
        Linux:x86_64|Linux:i?86)
            if [ -e /dev/kvm ]; then
                printf 'kvm\n'
            else
                printf 'tcg\n'
            fi
            ;;
        *)
            printf 'tcg\n'
            ;;
    esac
}

detect_default_display() {
    case "$(uname -s)" in
        Darwin) printf 'cocoa\n' ;;
        Linux) printf 'gtk\n' ;;
        *) printf 'default\n' ;;
    esac
}

QEMU_ACCEL="${QEMU_ACCEL:-$(detect_default_accel)}"
QEMU_DISPLAY="${QEMU_DISPLAY:-$(detect_default_display)}"

# Generate a small disk image for GRUB to boot from
# This assumes xorriso or similar is available to create an ISO,
# or we can use a raw disk image.
# For simplicity, use a raw image with GRUB installed.
# This requires significant setup, here is the qemu command:

exec qemu-system-x86_64 \
    -machine pc \
    -accel "$QEMU_ACCEL" \
    -m 512M \
    -display "$QEMU_DISPLAY" \
    -vga std \
    -kernel build/grub/xiaoos.bin \
    -monitor none \
    -no-reboot
