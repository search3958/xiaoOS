#!/usr/bin/env sh
set -eu

cd "$(dirname "$0")"
make bios

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

QEMU_ACCEL="${QEMU_ACCEL:-$(detect_default_accel)}"
QEMU_DISPLAY="${QEMU_DISPLAY:-}"
if [ -z "$QEMU_DISPLAY" ]; then
    case "$(uname -s)" in
        Darwin) QEMU_DISPLAY="cocoa" ;;
        Linux) QEMU_DISPLAY="gtk" ;;
        *) QEMU_DISPLAY="default" ;;
    esac
fi

find_file() {
    for path in "$@"; do
        if [ -n "$path" ] && [ -f "$path" ]; then
            printf '%s\n' "$path"
            return 0
        fi
    done
    return 1
}

exec qemu-system-i386 \
    -accel "$QEMU_ACCEL" \
    -m 512M \
    -display "$QEMU_DISPLAY" \
    -drive file=build/bios/xiao-bios.img,format=raw \
    -monitor none \
    -no-reboot
