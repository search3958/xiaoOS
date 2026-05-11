#!/usr/bin/env sh
set -eu

cd "$(dirname "$0")"
make arm64

find_firmware() {
    name="$1"
    for path in \
        "${QEMU_EFI_CODE:-}" \
        "${QEMU_SHARE:-}/$name" \
        "$(brew --prefix qemu 2>/dev/null || true)/share/qemu/$name" \
        "/opt/homebrew/share/qemu/$name" \
        "/usr/local/share/qemu/$name"
    do
        if [ -n "$path" ] && [ -f "$path" ]; then
            printf '%s\n' "$path"
            return 0
        fi
    done
    echo "AArch64 UEFI firmware not found: $name" >&2
    echo "Set QEMU_EFI_CODE=/path/to/$name and retry." >&2
    exit 1
}

code="$(find_firmware edk2-aarch64-code.fd)"

exec qemu-system-aarch64 \
    -M virt \
    -cpu cortex-a72 \
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
    -serial vc \
    -monitor none \
    -no-reboot
