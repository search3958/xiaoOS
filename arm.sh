#!/usr/bin/env sh
set -eu

cd "$(dirname "$0")"
make arm64

detect_default_accel() {
    os="$(uname -s)"
    arch="$(uname -m)"

    case "$os:$arch" in
        Darwin:arm64|Darwin:aarch64)
            printf 'hvf\n'
            ;;
        Linux:arm64|Linux:aarch64)
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
QEMU_CPU="${QEMU_CPU:-}"
if [ -z "$QEMU_CPU" ]; then
    case "$QEMU_ACCEL" in
        hvf|kvm)
            QEMU_CPU="host"
            ;;
        *)
            QEMU_CPU="max"
            ;;
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

brew_qemu_share=""
if command -v brew >/dev/null 2>&1; then
    brew_qemu_share="$(brew --prefix qemu 2>/dev/null || true)/share/qemu"
fi

code="$(find_file \
    "${QEMU_EFI_CODE:-}" \
    "${QEMU_SHARE:-}/edk2-aarch64-code.fd" \
    "${QEMU_SHARE:-}/QEMU_EFI.fd" \
    "${QEMU_SHARE:-}/AAVMF_CODE.fd" \
    "$brew_qemu_share/edk2-aarch64-code.fd" \
    "$brew_qemu_share/QEMU_EFI.fd" \
    "$brew_qemu_share/AAVMF_CODE.fd" \
    "/opt/homebrew/share/qemu/edk2-aarch64-code.fd" \
    "/usr/local/share/qemu/edk2-aarch64-code.fd" \
    "/usr/share/qemu/edk2-aarch64-code.fd" \
    "/usr/share/qemu-efi-aarch64/QEMU_EFI.fd" \
    "/usr/share/AAVMF/AAVMF_CODE.fd" \
    "/usr/share/AAVMF/AAVMF_CODE.ms.fd" \
    "/usr/share/edk2/aarch64/QEMU_EFI.fd" \
    "/usr/share/edk2/armvirt/QEMU_EFI.fd" \
    "/usr/share/edk2/aarch64/AAVMF_CODE.fd" \
    || true)"

if [ -z "$code" ]; then
    echo "AArch64 UEFI firmware not found." >&2
    echo "Set QEMU_EFI_CODE=/path/to/QEMU_EFI.fd and retry." >&2
    exit 1
fi

exec qemu-system-aarch64 \
    -M virt \
    -accel "$QEMU_ACCEL" \
    -cpu "$QEMU_CPU" \
    -m 512M \
    -device virtio-gpu-pci,xres=1280,yres=720 \
    -device qemu-xhci \
    -device usb-kbd \
    -device usb-tablet \
    -device virtio-keyboard-device \
    -device virtio-tablet-device \
    -bios "$code" \
    -drive if=none,id=hd0,file=fat:rw:build/arm64/esp,format=raw \
    -device virtio-blk-device,drive=hd0 \
    -net none \
    -serial stdio \
    -device usb-mouse \
    -monitor none \
    -no-reboot
