#!/usr/bin/env sh
set -eu

cd "$(dirname "$0")"

FQBN="${FQBN:-esp32:esp32:esp32s3:CDCOnBoot=cdc}"
BAUD="${BAUD:-115200}"
ACTION="${1:-upload-monitor}"
BUILD_EXTRA_FLAGS="${EXTRA_FLAGS:-}"

resolve_arduino_cli() {
    if [ -n "${ARDUINO_CLI:-}" ] && [ -x "${ARDUINO_CLI}" ]; then
        printf '%s\n' "${ARDUINO_CLI}"
        return 0
    fi

    if [ -x "./tools/bin/arduino-cli" ]; then
        printf '%s\n' "./tools/bin/arduino-cli"
        return 0
    fi

    if command -v arduino-cli >/dev/null 2>&1; then
        command -v arduino-cli
        return 0
    fi

    return 1
}

detect_port() {
    if [ -n "${PORT:-}" ]; then
        printf '%s\n' "$PORT"
        return 0
    fi

    for pattern in \
        /dev/cu.usbmodem* \
        /dev/cu.usbserial* \
        /dev/cu.SLAB_USBtoUART* \
        /dev/cu.wchusbserial* \
        /dev/ttyUSB* \
        /dev/ttyACM* \
        /dev/tty.SLAB_USBtoUART* \
        /dev/tty.wchusbserial* \
        /dev/serial/by-id/*
    do
        for port in $pattern; do
            if [ -e "$port" ]; then
                printf '%s\n' "$port"
                return 0
            fi
        done
    done

    echo "ESP32 serial port not found." >&2
    echo "Set PORT=/dev/ttyUSB0 (Linux) or PORT=/dev/cu.usbmodem* (macOS) and retry." >&2
    exit 1
}

require_arduino_cli() {
    if ! ARDUINO_CLI_BIN="$(resolve_arduino_cli)"; then
        echo "arduino-cli is required." >&2
        echo "Recommended: install local binary" >&2
        echo "mkdir -p tools/bin && curl -fsSL https://raw.githubusercontent.com/arduino/arduino-cli/master/install.sh | BINDIR=\"$PWD/tools/bin\" sh" >&2
        exit 127
    fi
}

require_arduino_cli

if [ -n "${ARDUINO_CLI_BIN:-}" ]; then
    "$ARDUINO_CLI_BIN" version >/dev/null 2>&1 || true
fi

python3 tools/sync_sketch.py hal/esp32c3/xiaoOS

case "$ACTION" in
    compile)
        if [ -n "$BUILD_EXTRA_FLAGS" ]; then
            "$ARDUINO_CLI_BIN" compile --fqbn "$FQBN" --build-property build.extra_flags="$BUILD_EXTRA_FLAGS" hal/esp32c3/xiaoOS
        else
            "$ARDUINO_CLI_BIN" compile --fqbn "$FQBN" hal/esp32c3/xiaoOS
        fi
        ;;
    upload)
        port="$(detect_port)"
        echo "Using $port with $FQBN"
        if [ -n "$BUILD_EXTRA_FLAGS" ]; then
            "$ARDUINO_CLI_BIN" compile --fqbn "$FQBN" --build-property build.extra_flags="$BUILD_EXTRA_FLAGS" --upload -p "$port" hal/esp32c3/xiaoOS
        else
            "$ARDUINO_CLI_BIN" compile --fqbn "$FQBN" --upload -p "$port" hal/esp32c3/xiaoOS
        fi
        ;;
    monitor)
        port="$(detect_port)"
        echo "Using $port at $BAUD baud"
        "$ARDUINO_CLI_BIN" monitor -p "$port" -c baudrate="$BAUD"
        ;;
    upload-monitor)
        port="$(detect_port)"
        echo "Using $port with $FQBN"
        if [ -n "$BUILD_EXTRA_FLAGS" ]; then
            "$ARDUINO_CLI_BIN" compile --fqbn "$FQBN" --build-property build.extra_flags="$BUILD_EXTRA_FLAGS" --upload -p "$port" hal/esp32c3/xiaoOS
        else
            "$ARDUINO_CLI_BIN" compile --fqbn "$FQBN" --upload -p "$port" hal/esp32c3/xiaoOS
        fi
        "$ARDUINO_CLI_BIN" monitor -p "$port" -c baudrate="$BAUD"
        ;;
    *)
        echo "usage: ./esp32c3.sh [compile|upload|monitor|upload-monitor]" >&2
        exit 2
        ;;
esac
