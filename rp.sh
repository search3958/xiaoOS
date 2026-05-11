#!/usr/bin/env sh
set -eu

cd "$(dirname "$0")"
make arm64

find_file() {
    for path in "$@"; do
        if [ -n "$path" ] && [ -f "$path" ]; then
            printf '%s\n' "$path"
            return 0
        fi
    done
    return 1
}

share_qemu="$(brew --prefix qemu 2>/dev/null || true)/share/qemu"
[ -d "$share_qemu" ] || share_qemu="/opt/homebrew/share/qemu"

rpi_efi="$(find_file \
    "${RPI_EFI_CODE:-}" \
    "${QEMU_SHARE:-}/RPI_EFI.fd" \
    "$share_qemu/RPI_EFI.fd" \
    "/usr/local/share/qemu/RPI_EFI.fd" \
    || true)"

if [ -n "$rpi_efi" ]; then
    echo "Using Raspberry Pi UEFI firmware: $rpi_efi"
    exec qemu-system-aarch64 \
        -M raspi3b \
        -cpu cortex-a53 \
        -m 1024 \
        -bios "$rpi_efi" \
        -drive if=none,id=usbdisk,file=fat:rw:build/arm64/esp,format=raw \
        -device usb-storage,drive=usbdisk \
        -device usb-kbd \
        -device usb-tablet \
        -serial vc \
        -monitor none \
        -no-reboot
fi

# Fallback profile: Pi2-class resources on virt machine for reliable boot with stock edk2 firmware.
aarch64_efi="$(find_file \
    "${QEMU_EFI_CODE:-}" \
    "${QEMU_SHARE:-}/edk2-aarch64-code.fd" \
    "$share_qemu/edk2-aarch64-code.fd" \
    "/usr/local/share/qemu/edk2-aarch64-code.fd" \
    || true)"

if [ -z "$aarch64_efi" ]; then
    echo "UEFI firmware not found. Install qemu edk2 firmware or set QEMU_EFI_CODE/RPI_EFI_CODE." >&2
    exit 1
fi

echo "RPI_EFI.fd not found, using compatible fallback (virt + cortex-a53, 1GB)."
exec qemu-system-aarch64 \
    -M virt \
    -cpu cortex-a53 \
    -smp 4 \
    -m 1024 \
    -device virtio-gpu-pci,xres=1280,yres=720 \
    -device qemu-xhci \
    -device usb-kbd \
    -device usb-tablet \
    -bios "$aarch64_efi" \
    -drive if=none,id=hd0,file=fat:rw:build/arm64/esp,format=raw \
    -device virtio-blk-device,drive=hd0 \
    -net none \
    -serial vc \
    -monitor none \
    -no-reboot
