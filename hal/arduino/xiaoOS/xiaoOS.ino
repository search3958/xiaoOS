#include <Arduino.h>

extern "C" {
#include "xiao.h"
}

static void arduino_serial_write(const char *data, xiao_size len) {
    xiao_size i;
    for (i = 0; i < len; i++) Serial.write(data[i]);
}

static void arduino_wait_ms(xiao_tick ms) {
    delay((unsigned long)ms);
}

static int arduino_input_read(void) {
    if (Serial.available() <= 0) return -1;
    return (int)Serial.read();
}

static void arduino_yield(void) {
    yield();
}

static const xiao_hal arduino_hal = {
    arduino_serial_write,
    arduino_serial_write,
    arduino_input_read,
    arduino_wait_ms,
    arduino_yield,
    0,
    0,
    0,
    0,
    0,
    XIAO_PLATFORM_UNKNOWN,
    0,
    0,
};

void setup() {
    Serial.begin(115200);
    while (!Serial && millis() < 3000) {}
    xiao_start(&arduino_hal, &xiao_image);
}

void loop() {
}
