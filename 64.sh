#!/usr/bin/env sh
set -eu

cd "$(dirname "$0")"
make uefi

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
    echo "UEFI firmware not found: $name" >&2
    echo "Set QEMU_EFI_CODE=/path/to/$name and retry." >&2
    exit 1
}

code="$(find_firmware edk2-x86_64-code.fd)"

exec qemu-system-x86_64 \
    -machine q35 \
    -drive if=pflash,format=raw,readonly=on,file="$code" \
    -drive if=ide,file=fat:rw:build/uefi/esp,format=raw \
    -serial vc \
    -monitor none \
    -no-reboot
