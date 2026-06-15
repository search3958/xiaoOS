#!/usr/bin/env sh
set -eu

cd "$(dirname "$0")"

mode="${1:-uefi}"
case "$mode" in
    bios|uefi) ;;
    *)
        echo "usage: $0 [bios|uefi]" >&2
        exit 1
        ;;
esac

if [ "$mode" = bios ]; then
    make bios
else
    make uefi
fi

detect_default_accel() {
    os="$(uname -s)"
    arch="$(uname -m)"

    case "$os:$arch" in
        Darwin:x86_64)
            printf 'hvf\n'
            ;;
        Linux:x86_64)
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

find_file() {
    for path in "$@"; do
        if [ -n "$path" ] && [ -f "$path" ]; then
            printf '%s\n' "$path"
            return 0
        fi
    done
    return 1
}

if [ "$mode" = bios ]; then
    exec qemu-system-i386 \
        -machine pc \
        -accel "$QEMU_ACCEL" \
        -m 512M \
        -display "$QEMU_DISPLAY" \
        -vga std \
        -drive file=build/bios/xiao-bios.img,format=raw \
        -device usb-ehci \
        -device usb-tablet \
        -device usb-mouse \
        -device usb-kbd \
        -serial stdio \
        -monitor none \
        -no-reboot
fi

brew_qemu_share=""
if command -v brew >/dev/null 2>&1; then
    brew_qemu_share="$(brew --prefix qemu 2>/dev/null || true)/share/qemu"
fi

code="$(find_file \
    "${QEMU_EFI_CODE:-}" \
    "${QEMU_SHARE:-}/edk2-x86_64-code.fd" \
    "${QEMU_SHARE:-}/OVMF_CODE.fd" \
    "${QEMU_SHARE:-}/OVMF_CODE_4M.fd" \
    "$brew_qemu_share/edk2-x86_64-code.fd" \
    "$brew_qemu_share/OVMF_CODE.fd" \
    "$brew_qemu_share/OVMF_CODE_4M.fd" \
    "/opt/homebrew/share/qemu/edk2-x86_64-code.fd" \
    "/opt/homebrew/share/qemu/OVMF_CODE.fd" \
    "/opt/homebrew/share/qemu/OVMF_CODE_4M.fd" \
    "/usr/local/share/qemu/edk2-x86_64-code.fd" \
    "/usr/local/share/qemu/OVMF_CODE.fd" \
    "/usr/local/share/qemu/OVMF_CODE_4M.fd" \
    "/usr/share/qemu/edk2-x86_64-code.fd" \
    "/usr/share/qemu/OVMF_CODE.fd" \
    "/usr/share/qemu/OVMF_CODE_4M.fd" \
    "/usr/share/OVMF/OVMF_CODE.fd" \
    "/usr/share/OVMF/OVMF_CODE_4M.fd" \
    "/usr/share/edk2/ovmf/OVMF_CODE.fd" \
    "/usr/share/edk2/ovmf/OVMF_CODE.4m.fd" \
    "/usr/share/edk2/x64/OVMF_CODE.fd" \
    "/usr/share/edk2/x64/OVMF_CODE.4m.fd" \
    || true)"

if [ -z "$code" ]; then
    echo "x86_64 UEFI firmware not found." >&2
    echo "Set QEMU_EFI_CODE=/path/to/OVMF_CODE.fd and retry." >&2
    exit 1
fi

exec qemu-system-x86_64 \
    -machine q35 \
    -accel "$QEMU_ACCEL" \
    -m 512M \
    -display "$QEMU_DISPLAY" \
    -drive if=pflash,format=raw,readonly=on,file="$code" \
    -drive if=ide,file=fat:rw:build/uefi/esp,format=raw \
    -device qemu-xhci \
    -device usb-tablet \
    -device usb-mouse \
    -device usb-kbd \
    -serial stdio \
    -monitor none \
    -no-reboot
