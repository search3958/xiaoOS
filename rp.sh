#!/usr/bin/env sh
set -eu

cd "$(dirname "$0")"

usage() {
    cat <<'EOF'
Usage: $0 [image|qemu]

Commands:
  image   Generate a bootable SD card image for Raspberry Pi 4 (default)
  qemu    Run in QEMU raspi3b emulator (for testing)

Environment variables:
  RPI_UBOOT_DIR      Path to U-Boot binary for RPi 4 (REQUIRED for image)
                     Must contain: u-boot.bin (UEFI-capable build)
                     Build: make rpi_4_defconfig && make
                     Or download from: https://ftp.denx.de/pub/u-boot/
  RPI_FIRMWARE_DIR   Path to standard RPi firmware (start4.elf, fixup4.dat, etc.)
                     Download from: https://github.com/raspberrypi/firmware/tree/master/boot
  QEMU_ACCEL         QEMU acceleration (hvf/kvm/tcg)
  QEMU_DISPLAY       QEMU display backend (cocoa/gtk/default)

Boot process: bootcode.bin -> start4.elf -> U-Boot -> UEFI -> BOOTAA64.EFI
Image size is auto-calculated from content (minimum 8MB).
EOF
    exit 1
}

cmd="${1:-image}"
case "$cmd" in
    image) ;;
    qemu) ;;
    -h|--help|help) usage ;;
    *) echo "Unknown command: $cmd" >&2; usage ;;
esac

# ── build arm64 UEFI application ──────────────────────────────────────────────
make arm64

# ── shared helpers ────────────────────────────────────────────────────────────
find_file() {
    for path in "$@"; do
        if [ -n "$path" ] && [ -f "$path" ]; then
            printf '%s\n' "$path"
            return 0
        fi
    done
    return 1
}

detect_default_accel() {
    os="$(uname -s)"
    arch="$(uname -m)"
    case "$os:$arch" in
        Darwin:arm64|Darwin:aarch64) printf 'hvf\n' ;;
        Linux:arm64|Linux:aarch64)
            if [ -e /dev/kvm ]; then printf 'kvm\n'; else printf 'tcg\n'; fi ;;
        *) printf 'tcg\n' ;;
    esac
}

detect_default_display() {
    case "$(uname -s)" in
        Darwin) printf 'cocoa\n' ;;
        Linux) printf 'gtk\n' ;;
        *) printf 'default\n' ;;
    esac
}

# ── QEMU mode ────────────────────────────────────────────────────────────────
if [ "$cmd" = qemu ]; then
    QEMU_ACCEL="${QEMU_ACCEL:-$(detect_default_accel)}"
    QEMU_CPU="${QEMU_CPU:-}"
    if [ -z "$QEMU_CPU" ]; then
        case "$QEMU_ACCEL" in
            hvf|kvm) QEMU_CPU="host" ;;
            *) QEMU_CPU="max" ;;
        esac
    fi
    QEMU_DISPLAY="${QEMU_DISPLAY:-$(detect_default_display)}"

    brew_qemu_share=""
    if command -v brew >/dev/null 2>&1; then
        brew_qemu_share="$(brew --prefix qemu 2>/dev/null || true)/share/qemu"
    fi

    aarch64_efi="$(find_file \
        "${QEMU_EFI_CODE:-}" \
        "${QEMU_SHARE:-}/edk2-aarch64-code.fd" \
        "${QEMU_SHARE:-}/QEMU_EFI.fd" \
        "$brew_qemu_share/edk2-aarch64-code.fd" \
        "$brew_qemu_share/QEMU_EFI.fd" \
        "/opt/homebrew/share/qemu/edk2-aarch64-code.fd" \
        "/usr/local/share/qemu/edk2-aarch64-code.fd" \
        "/usr/share/qemu/edk2-aarch64-code.fd" \
        "/usr/share/AAVMF/AAVMF_CODE.fd" \
        || true)"

    if [ -z "$aarch64_efi" ]; then
        echo "UEFI firmware not found. Set QEMU_EFI_CODE." >&2
        exit 1
    fi

    # QEMU raspi4b firmware is broken; use virt machine with UEFI instead
    exec qemu-system-aarch64 \
        -M virt \
        -accel "$QEMU_ACCEL" \
        -cpu "$QEMU_CPU" \
        -smp 4 \
        -m 1024M \
        -display "$QEMU_DISPLAY" \
        -device virtio-gpu-pci,xres=1280,yres=720 \
        -device qemu-xhci \
        -device usb-kbd \
        -device usb-tablet \
        -device usb-mouse \
        -bios "$aarch64_efi" \
        -drive if=none,id=hd0,file=fat:rw:build/arm64/esp,format=raw \
        -device virtio-blk-device,drive=hd0 \
        -net none \
        -serial stdio \
        -monitor none \
        -no-reboot
fi

# ── image mode: generate SD card image for Raspberry Pi 4 ────────────────────

OUTPUT="build/rpi4/xiaoOS-rpi4.img"
BOOT_DIR="build/rpi4/boot"

rm -rf build/rpi4
mkdir -p "$BOOT_DIR/EFI/BOOT"

# copy EFI application
cp build/arm64/esp/EFI/BOOT/BOOTAA64.EFI "$BOOT_DIR/EFI/BOOT/"

# create RPi 4 config.txt
cat > "$BOOT_DIR/config.txt" <<'CONF'
# xiaoOS for Raspberry Pi 4
arm_64bit=1
gpu_mem=64
enable_uart=1
boot_delay=0
CONF

# ── locate U-Boot for RPi 4 ─────────────────────────────────────────────────
# U-Boot provides UEFI compatibility via CONFIG_EFI_LOADER
# Boot: bootcode4.bin -> start4.elf -> U-Boot -> UEFI -> BOOTAA64.EFI

UBOOT_SEARCH_PATHS="
    ${RPI_UBOOT_DIR:-}
    tools/rpi4-uboot/bin
    tools/rpi4-uboot/src
    $HOME/u-boot
    $HOME/rpi4-u-boot
    /opt/u-boot
    /usr/local/share/u-boot/u-boot-rpi4
"

uboot_found=0
while IFS= read -r dir; do
    dir="$(echo "$dir" | tr -d '[:space:]')"
    [ -z "$dir" ] && continue
    # U-Boot for RPi 4 can be u-boot.bin or u-boot.bin.lzma
    for name in u-boot.bin u-boot.bin.lzma; do
        if [ -f "$dir/$name" ]; then
            # RPi 4 expects kernel8.img for 64-bit boot
            cp "$dir/$name" "$BOOT_DIR/kernel8.img"
            uboot_found=1
            break 2
        fi
    done
done <<EOF
$UBOOT_SEARCH_PATHS
EOF

if [ "$uboot_found" -eq 0 ]; then
    echo "Error: U-Boot binary not found." >&2
    echo "" >&2
    echo "U-Boot is required for UEFI boot on Raspberry Pi 4." >&2
    echo "" >&2
    echo "Options:" >&2
    echo "  1. Download prebuilt:" >&2
    echo "     https://ftp.denx.de/pub/u-boot/" >&2
    echo "     Or from your distro: apt install u-boot-rpi4 (Debian/Ubuntu)" >&2
    echo "" >&2
    echo "  2. Build from source:" >&2
    echo "     git clone https://source.denx.de/u-boot/u-boot.git" >&2
    echo "     cd u-boot" >&2
    echo "     make rpi_4_defconfig" >&2
    echo "     make -j\$(nproc)" >&2
    echo "" >&2
    echo "Required: u-boot.bin with UEFI support (CONFIG_EFI_LOADER=y)" >&2
    echo "Set RPI_UBOOT_DIR=/path/to/u-boot and retry." >&2
    exit 1
fi

# ── locate standard RPi firmware (start4.elf, fixup4.dat, bootcode.bin) ────
FW_SEARCH_PATHS="
    ${RPI_FIRMWARE_DIR:-}
    tools/rpi4-firmware
    /boot
    /boot/firmware
    /Volumes/boot
    $HOME/rpi-firmware/boot
    $HOME/firmware/boot
"

# RPi 4 uses bootcode.bin (not bootcode4.bin)
RPi4_FW="start4.elf fixup4.dat bootcode.bin"

for fw in $RPi4_FW; do
    [ -f "$BOOT_DIR/$fw" ] && continue
    while IFS= read -r dir; do
        dir="$(echo "$dir" | tr -d '[:space:]')"
        [ -z "$dir" ] && continue
        if [ -f "$dir/$fw" ]; then
            cp "$dir/$fw" "$BOOT_DIR/"
            break
        fi
    done <<EOF
$FW_SEARCH_PATHS
EOF
done

# also accept bootcode4.bin as alternative name
if [ ! -f "$BOOT_DIR/bootcode.bin" ] && [ -f "$BOOT_DIR/bootcode4.bin" ]; then
    cp "$BOOT_DIR/bootcode4.bin" "$BOOT_DIR/bootcode.bin"
fi

# verify all required files are present
echo "Boot partition contents:"
missing=0
for f in kernel8.img start4.elf fixup4.dat bootcode.bin config.txt; do
    if [ -f "$BOOT_DIR/$f" ]; then
        echo "  $f ($(wc -c < "$BOOT_DIR/$f") bytes)"
    else
        echo "  $f MISSING" >&2
        missing=1
    fi
done
echo "  EFI/BOOT/BOOTAA64.EFI ($(wc -c < "$BOOT_DIR/EFI/BOOT/BOOTAA64.EFI") bytes)"

if [ "$missing" -eq 1 ]; then
    echo "" >&2
    echo "Missing required firmware files. Set RPI_FIRMWARE_DIR and retry." >&2
    exit 1
fi

# ── create FAT32 image with MBR ──────────────────────────────────────────────

require_tool() {
    command -v "$1" >/dev/null 2>&1 || {
        echo "Required tool not found: $1" >&2
        case "$(uname -s)" in
            Darwin) echo "Install via: brew install $2" >&2 ;;
            Linux)  echo "Install via: sudo apt install $2" >&2 ;;
        esac
        exit 1
    }
}

require_tool mkfs.fat dosfstools
require_tool mcopy mtools

mkdir -p "$(dirname "$OUTPUT")"

# 64MB image (FAT32 minimum)
dd if=/dev/zero of="$OUTPUT" bs=1M count=64 2>/dev/null
mkfs.fat -F 32 -s 8 -S 512 -n XIAO_OS "$OUTPUT" 2>/dev/null

# Copy files individually (mcopy -s recursive is broken on some versions)
mcopy -i "$OUTPUT" "$BOOT_DIR"/bootcode.bin ::bootcode.bin
mcopy -i "$OUTPUT" "$BOOT_DIR"/config.txt ::config.txt
mcopy -i "$OUTPUT" "$BOOT_DIR"/fixup4.dat ::fixup4.dat
mcopy -i "$OUTPUT" "$BOOT_DIR"/kernel8.img ::kernel8.img
mcopy -i "$OUTPUT" "$BOOT_DIR"/start4.elf ::start4.elf
# Copy EFI directory tree manually
mmd -i "$OUTPUT" ::EFI
mmd -i "$OUTPUT" ::EFI/BOOT
mcopy -i "$OUTPUT" "$BOOT_DIR/EFI/BOOT/BOOTAA64.EFI" ::EFI/BOOT/BOOTAA64.EFI

echo ""
echo "Image created: $OUTPUT"
echo ""
echo "Boot chain: start4.elf -> kernel8.img (U-Boot) -> UEFI -> BOOTAA64.EFI"
echo ""
echo "To flash to SD card or USB:"
echo "  sudo dd if=$OUTPUT of=/dev/sdX bs=4M status=progress && sync"
echo ""
echo "To test in QEMU (virt machine):"
echo "  ./arm.sh"
