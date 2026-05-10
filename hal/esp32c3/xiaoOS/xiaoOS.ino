#include <Arduino.h>

extern "C" {
#include "xiao.h"
}

static void esp32c3_serial_write(const char *data, xiao_size len) {
    xiao_size i;
    for (i = 0; i < len; i++) Serial.write(data[i]);
    Serial.flush();
}

static void esp32c3_wait_ms(xiao_tick ms) {
    delay((unsigned long)ms);
}

static void esp32c3_yield(void) {
    yield();
}

static const xiao_hal esp32c3_hal = {
    esp32c3_serial_write,
    esp32c3_serial_write,
    esp32c3_wait_ms,
    esp32c3_yield,
};

void setup() {
    Serial.begin(115200);
    unsigned long start = millis();
    while (!Serial && millis() - start < 3000) {
        delay(10);
    }
    xiao_start(&esp32c3_hal, &xiao_image);
}

void loop() {
}
