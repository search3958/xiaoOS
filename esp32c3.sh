#!/usr/bin/env sh
set -eu

cd "$(dirname "$0")"

FQBN="${FQBN:-esp32:esp32:esp32s3:CDCOnBoot=cdc}"
BAUD="${BAUD:-115200}"
ACTION="${1:-upload-monitor}"
DEFAULT_EXTRA_FLAGS="-DXIAO_TTF_TERMINAL_DISABLED -I$PWD/include -I$PWD/apps -I$PWD/third_party/litehtml/include -I$PWD/third_party/litehtml/include/litehtml -I$PWD/third_party/litehtml/src -I$PWD/third_party/litehtml/src/gumbo -I$PWD/third_party/litehtml/src/gumbo/include -I$PWD/third_party/litehtml/src/gumbo/include/gumbo"
DEFAULT_CPP_EXTRA_FLAGS=""
if [ -n "${EXTRA_FLAGS:-}" ]; then
    BUILD_EXTRA_FLAGS="$DEFAULT_EXTRA_FLAGS $EXTRA_FLAGS"
else
    BUILD_EXTRA_FLAGS="$DEFAULT_EXTRA_FLAGS"
fi
if [ -n "${CPP_EXTRA_FLAGS:-}" ]; then
    BUILD_CPP_EXTRA_FLAGS="$DEFAULT_CPP_EXTRA_FLAGS $CPP_EXTRA_FLAGS"
else
    BUILD_CPP_EXTRA_FLAGS="$DEFAULT_CPP_EXTRA_FLAGS"
fi

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
        /dev/serial/by-id/* \
        /dev/cu.usbmodem* \
        /dev/cu.usbserial* \
        /dev/cu.SLAB_USBtoUART* \
        /dev/cu.wchusbserial* \
        /dev/ttyUSB* \
        /dev/ttyACM* \
        /dev/tty.SLAB_USBtoUART* \
        /dev/tty.wchusbserial*
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

release_stale_monitor() {
    port="$1"
    if ! command -v lsof >/dev/null 2>&1; then
        return 0
    fi

    pids="$(lsof -t "$port" 2>/dev/null || true)"
    [ -n "$pids" ] || return 0

    for pid in $pids; do
        comm="$(ps -p "$pid" -o comm= 2>/dev/null | tr -d '[:space:]' || true)"
        if [ "$comm" = "serial-monitor" ]; then
            echo "Releasing stale serial monitor on $port (pid $pid)"
            kill "$pid" 2>/dev/null || true
            sleep 1
            if kill -0 "$pid" 2>/dev/null; then
                kill -9 "$pid" 2>/dev/null || true
            fi
        fi
    done
}

run_monitor() {
    port="$1"
    if python3 - <<'PY' >/dev/null 2>&1
import importlib.util, sys
sys.exit(0 if importlib.util.find_spec("serial.tools.miniterm") else 1)
PY
    then
        echo "Starting pyserial miniterm on $port at $BAUD baud"
        exec python3 -m serial.tools.miniterm "$port" "$BAUD" --raw --dtr 0 --rts 0
    fi

    if "$ARDUINO_CLI_BIN" monitor -p "$port" -c baudrate="$BAUD"; then
        return 0
    fi

    echo "monitor failed and no fallback available (install pyserial)" >&2
    return 1
}

force_run_application() {
    port="$1"
    esptool_bin="$(find "$HOME/.arduino15/packages/esp32/tools/esptool_py" -type f -name esptool 2>/dev/null | sort | tail -n 1)"
    if [ -z "$esptool_bin" ] || [ ! -x "$esptool_bin" ]; then
        return 0
    fi
    "$esptool_bin" --chip esp32s3 --port "$port" --before no-reset --after watchdog-reset run >/dev/null 2>&1 || true
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
        if [ -n "$BUILD_EXTRA_FLAGS" ] || [ -n "$BUILD_CPP_EXTRA_FLAGS" ]; then
            if [ -n "$BUILD_EXTRA_FLAGS" ] && [ -n "$BUILD_CPP_EXTRA_FLAGS" ]; then
                "$ARDUINO_CLI_BIN" compile --fqbn "$FQBN" --build-property build.extra_flags="$BUILD_EXTRA_FLAGS" --build-property compiler.cpp.extra_flags="$BUILD_CPP_EXTRA_FLAGS" hal/esp32c3/xiaoOS
            elif [ -n "$BUILD_EXTRA_FLAGS" ]; then
                "$ARDUINO_CLI_BIN" compile --fqbn "$FQBN" --build-property build.extra_flags="$BUILD_EXTRA_FLAGS" hal/esp32c3/xiaoOS
            else
                "$ARDUINO_CLI_BIN" compile --fqbn "$FQBN" --build-property compiler.cpp.extra_flags="$BUILD_CPP_EXTRA_FLAGS" hal/esp32c3/xiaoOS
            fi
        else
            "$ARDUINO_CLI_BIN" compile --fqbn "$FQBN" hal/esp32c3/xiaoOS
        fi
        ;;
    upload)
        port="$(detect_port)"
        release_stale_monitor "$port"
        echo "Using $port with $FQBN"
        if [ -n "$BUILD_EXTRA_FLAGS" ] || [ -n "$BUILD_CPP_EXTRA_FLAGS" ]; then
            if [ -n "$BUILD_EXTRA_FLAGS" ] && [ -n "$BUILD_CPP_EXTRA_FLAGS" ]; then
                "$ARDUINO_CLI_BIN" compile --fqbn "$FQBN" --build-property build.extra_flags="$BUILD_EXTRA_FLAGS" --build-property compiler.cpp.extra_flags="$BUILD_CPP_EXTRA_FLAGS" --upload -p "$port" hal/esp32c3/xiaoOS
            elif [ -n "$BUILD_EXTRA_FLAGS" ]; then
                "$ARDUINO_CLI_BIN" compile --fqbn "$FQBN" --build-property build.extra_flags="$BUILD_EXTRA_FLAGS" --upload -p "$port" hal/esp32c3/xiaoOS
            else
                "$ARDUINO_CLI_BIN" compile --fqbn "$FQBN" --build-property compiler.cpp.extra_flags="$BUILD_CPP_EXTRA_FLAGS" --upload -p "$port" hal/esp32c3/xiaoOS
            fi
        else
            "$ARDUINO_CLI_BIN" compile --fqbn "$FQBN" --upload -p "$port" hal/esp32c3/xiaoOS
        fi
        force_run_application "$port"
        ;;
    monitor)
        port="$(detect_port)"
        release_stale_monitor "$port"
        echo "Using $port at $BAUD baud"
        run_monitor "$port"
        ;;
    upload-monitor)
        port="$(detect_port)"
        release_stale_monitor "$port"
        echo "Using $port with $FQBN"
        if [ -n "$BUILD_EXTRA_FLAGS" ] || [ -n "$BUILD_CPP_EXTRA_FLAGS" ]; then
            if [ -n "$BUILD_EXTRA_FLAGS" ] && [ -n "$BUILD_CPP_EXTRA_FLAGS" ]; then
                "$ARDUINO_CLI_BIN" compile --fqbn "$FQBN" --build-property build.extra_flags="$BUILD_EXTRA_FLAGS" --build-property compiler.cpp.extra_flags="$BUILD_CPP_EXTRA_FLAGS" --upload -p "$port" hal/esp32c3/xiaoOS
            elif [ -n "$BUILD_EXTRA_FLAGS" ]; then
                "$ARDUINO_CLI_BIN" compile --fqbn "$FQBN" --build-property build.extra_flags="$BUILD_EXTRA_FLAGS" --upload -p "$port" hal/esp32c3/xiaoOS
            else
                "$ARDUINO_CLI_BIN" compile --fqbn "$FQBN" --build-property compiler.cpp.extra_flags="$BUILD_CPP_EXTRA_FLAGS" --upload -p "$port" hal/esp32c3/xiaoOS
            fi
        else
            "$ARDUINO_CLI_BIN" compile --fqbn "$FQBN" --upload -p "$port" hal/esp32c3/xiaoOS
        fi
        force_run_application "$port"
        run_monitor "$port"
        ;;
    *)
        echo "usage: ./esp32c3.sh [compile|upload|monitor|upload-monitor]" >&2
        exit 2
        ;;
esac
