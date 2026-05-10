#!/usr/bin/env sh
set -eu

cd "$(dirname "$0")"

FQBN="${FQBN:-esp32:esp32:esp32c3:CDCOnBoot=cdc}"
BAUD="${BAUD:-115200}"
ACTION="${1:-upload-monitor}"

detect_port() {
    if [ -n "${PORT:-}" ]; then
        printf '%s\n' "$PORT"
        return 0
    fi
    for pattern in /dev/cu.usbmodem* /dev/cu.usbserial* /dev/cu.SLAB_USBtoUART* /dev/cu.wchusbserial*; do
        for port in $pattern; do
            if [ -e "$port" ]; then
                printf '%s\n' "$port"
                return 0
            fi
        done
    done
    echo "ESP32-C3 serial port not found. Set PORT=/dev/cu.xxx and retry." >&2
    exit 1
}

require_arduino_cli() {
    if ! command -v arduino-cli >/dev/null 2>&1; then
        echo "arduino-cli is required." >&2
        echo "Install on macOS: brew install arduino-cli" >&2
        echo "Then install ESP32 core:" >&2
        echo "arduino-cli config init" >&2
        echo "arduino-cli config add board_manager.additional_urls https://espressif.github.io/arduino-esp32/package_esp32_index.json" >&2
        echo "arduino-cli core update-index" >&2
        echo "arduino-cli core install esp32:esp32" >&2
        exit 127
    fi
}

require_arduino_cli
python3 tools/sync_sketch.py hal/esp32c3/xiaoOS

case "$ACTION" in
    compile)
        arduino-cli compile --fqbn "$FQBN" hal/esp32c3/xiaoOS
        ;;
    upload)
        port="$(detect_port)"
        echo "Using $port with $FQBN"
        arduino-cli compile --fqbn "$FQBN" --upload -p "$port" hal/esp32c3/xiaoOS
        ;;
    monitor)
        port="$(detect_port)"
        echo "Using $port at $BAUD baud"
        arduino-cli monitor -p "$port" -c baudrate="$BAUD"
        ;;
    upload-monitor)
        port="$(detect_port)"
        echo "Using $port with $FQBN"
        arduino-cli compile --fqbn "$FQBN" --upload -p "$port" hal/esp32c3/xiaoOS
        arduino-cli monitor -p "$port" -c baudrate="$BAUD"
        ;;
    *)
        echo "usage: ./esp32c3.sh [compile|upload|monitor|upload-monitor]" >&2
        exit 2
        ;;
esac
