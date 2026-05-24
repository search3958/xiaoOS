#include <Arduino.h>
#include <SPI.h>

extern "C" {
#include "xiao.h"
}

static int esp32c3_serial_connected = 0;

static void esp32c3_serial_write(const char *data, xiao_size len) {
    xiao_size i;
    if (Serial) esp32c3_serial_connected = 1;
    if (!esp32c3_serial_connected) return;
    for (i = 0; i < len; i++) Serial.write(data[i]);
}

static void esp32c3_wait_ms(xiao_tick ms) {
    delay((unsigned long)ms);
}

static int esp32c3_input_read(void) {
    if (Serial) esp32c3_serial_connected = 1;
    if (!esp32c3_serial_connected) return -1;
    if (Serial.available() <= 0) return -1;
    return (int)Serial.read();
}

extern "C" int xiao_esp32_serial_connected(void) {
    return esp32c3_serial_connected;
}

static void esp32c3_yield(void) {
    yield();
}

#ifndef XIAO_GC9A01_PIN_CS
#if defined(CONFIG_IDF_TARGET_ESP32S3)
#define XIAO_GC9A01_PIN_CS 10
#else
#define XIAO_GC9A01_PIN_CS 5
#endif
#endif
#ifndef XIAO_GC9A01_PIN_DC
#if defined(CONFIG_IDF_TARGET_ESP32S3)
#define XIAO_GC9A01_PIN_DC 4
#else
#define XIAO_GC9A01_PIN_DC 4
#endif
#endif
#ifndef XIAO_GC9A01_PIN_RST
#if defined(CONFIG_IDF_TARGET_ESP32S3)
#define XIAO_GC9A01_PIN_RST 2
#else
#define XIAO_GC9A01_PIN_RST -1
#endif
#endif
#ifndef XIAO_GC9A01_PIN_SCK
#if defined(CONFIG_IDF_TARGET_ESP32S3)
#define XIAO_GC9A01_PIN_SCK 36
#else
#define XIAO_GC9A01_PIN_SCK 8
#endif
#endif
#ifndef XIAO_GC9A01_PIN_MOSI
#if defined(CONFIG_IDF_TARGET_ESP32S3)
#define XIAO_GC9A01_PIN_MOSI 35
#else
#define XIAO_GC9A01_PIN_MOSI 10
#endif
#endif
#ifndef XIAO_GC9A01_SPI_HZ
#define XIAO_GC9A01_SPI_HZ 45000000UL
#endif

static int gc9a01_ready = 0;
#if defined(FSPI)
static SPIClass gc9a01_spi(FSPI);
#elif defined(HSPI)
static SPIClass gc9a01_spi(HSPI);
#else
static SPIClass gc9a01_spi;
#endif

#define XIAO_GC9A01_WIDTH 240
#define XIAO_GC9A01_HEIGHT 240

static const SPISettings gc9a01_spi_settings(XIAO_GC9A01_SPI_HZ, MSBFIRST, SPI_MODE0);
static uint8_t gc9a01_linebuf[XIAO_GC9A01_WIDTH * 2];

typedef struct {
    uint8_t cmd;
    const uint8_t *data;
    uint8_t size;
    uint16_t delay_ms;
} gc9a01_cmd;

static const uint8_t d_eb[] = {0x14};
static const uint8_t d_84[] = {0x60};
static const uint8_t d_85[] = {0xff};
static const uint8_t d_86[] = {0xff};
static const uint8_t d_87[] = {0xff};
static const uint8_t d_8e[] = {0xff};
static const uint8_t d_8f[] = {0xff};
static const uint8_t d_88[] = {0x0a};
static const uint8_t d_89[] = {0x23};
static const uint8_t d_8a[] = {0x00};
static const uint8_t d_8b[] = {0x80};
static const uint8_t d_8c[] = {0x01};
static const uint8_t d_8d[] = {0x03};
static const uint8_t d_90[] = {0x08, 0x08, 0x08, 0x08};
static const uint8_t d_ff[] = {0x60, 0x01, 0x04};
static const uint8_t d_c3[] = {0x13};
static const uint8_t d_c4[] = {0x13};
static const uint8_t d_c9[] = {0x30};
static const uint8_t d_be[] = {0x11};
static const uint8_t d_e1[] = {0x10, 0x0e};
static const uint8_t d_df[] = {0x21, 0x0c, 0x02};
static const uint8_t d_f0[] = {0x45, 0x09, 0x08, 0x08, 0x26, 0x2a};
static const uint8_t d_f1[] = {0x43, 0x70, 0x72, 0x36, 0x37, 0x6f};
static const uint8_t d_f2[] = {0x45, 0x09, 0x08, 0x08, 0x26, 0x2a};
static const uint8_t d_f3[] = {0x43, 0x70, 0x72, 0x36, 0x37, 0x6f};
static const uint8_t d_ed[] = {0x1b, 0x0b};
static const uint8_t d_ae[] = {0x77};
static const uint8_t d_cd[] = {0x63};
static const uint8_t d_70[] = {0x07, 0x07, 0x04, 0x0e, 0x0f, 0x09, 0x07, 0x08, 0x03};
static const uint8_t d_e8[] = {0x34};
static const uint8_t d_60[] = {0x38, 0x0b, 0x6d, 0x6d, 0x39, 0xf0, 0x6d, 0x6d};
static const uint8_t d_61[] = {0x38, 0xf4, 0x6d, 0x6d, 0x38, 0xf7, 0x6d, 0x6d};
static const uint8_t d_62[] = {0x38, 0x0d, 0x71, 0xed, 0x70, 0x70, 0x38, 0x0f, 0x71, 0xef, 0x70, 0x70};
static const uint8_t d_63[] = {0x38, 0x11, 0x71, 0xf1, 0x70, 0x70, 0x38, 0x13, 0x71, 0xf3, 0x70, 0x70};
static const uint8_t d_64[] = {0x28, 0x29, 0xf1, 0x01, 0xf1, 0x00, 0x07};
static const uint8_t d_66[] = {0x3c, 0x00, 0xcd, 0x67, 0x45, 0x45, 0x10, 0x00, 0x00, 0x00};
static const uint8_t d_67[] = {0x00, 0x3c, 0x00, 0x00, 0x00, 0x01, 0x54, 0x10, 0x32, 0x98};
static const uint8_t d_74[] = {0x10, 0x45, 0x80, 0x00, 0x00, 0x4e, 0x00};
static const uint8_t d_98[] = {0x3e, 0x07};
static const uint8_t d_99[] = {0x3e, 0x07};

static const gc9a01_cmd gc9a01_init_cmds[] = {
    {0xfe, 0, 0, 0},
    {0xef, 0, 0, 0},
    {0xeb, d_eb, sizeof(d_eb), 0},
    {0x84, d_84, sizeof(d_84), 0},
    {0x85, d_85, sizeof(d_85), 0},
    {0x86, d_86, sizeof(d_86), 0},
    {0x87, d_87, sizeof(d_87), 0},
    {0x8e, d_8e, sizeof(d_8e), 0},
    {0x8f, d_8f, sizeof(d_8f), 0},
    {0x88, d_88, sizeof(d_88), 0},
    {0x89, d_89, sizeof(d_89), 0},
    {0x8a, d_8a, sizeof(d_8a), 0},
    {0x8b, d_8b, sizeof(d_8b), 0},
    {0x8c, d_8c, sizeof(d_8c), 0},
    {0x8d, d_8d, sizeof(d_8d), 0},
    {0x90, d_90, sizeof(d_90), 0},
    {0xff, d_ff, sizeof(d_ff), 0},
    {0xc3, d_c3, sizeof(d_c3), 0},
    {0xc4, d_c4, sizeof(d_c4), 0},
    {0xc9, d_c9, sizeof(d_c9), 0},
    {0xbe, d_be, sizeof(d_be), 0},
    {0xe1, d_e1, sizeof(d_e1), 0},
    {0xdf, d_df, sizeof(d_df), 0},
    {0xf0, d_f0, sizeof(d_f0), 0},
    {0xf1, d_f1, sizeof(d_f1), 0},
    {0xf2, d_f2, sizeof(d_f2), 0},
    {0xf3, d_f3, sizeof(d_f3), 0},
    {0xed, d_ed, sizeof(d_ed), 0},
    {0xae, d_ae, sizeof(d_ae), 0},
    {0xcd, d_cd, sizeof(d_cd), 0},
    {0x70, d_70, sizeof(d_70), 0},
    {0xe8, d_e8, sizeof(d_e8), 0},
    {0x60, d_60, sizeof(d_60), 0},
    {0x61, d_61, sizeof(d_61), 0},
    {0x62, d_62, sizeof(d_62), 0},
    {0x63, d_63, sizeof(d_63), 0},
    {0x64, d_64, sizeof(d_64), 0},
    {0x66, d_66, sizeof(d_66), 0},
    {0x67, d_67, sizeof(d_67), 0},
    {0x74, d_74, sizeof(d_74), 0},
    {0x98, d_98, sizeof(d_98), 0},
    {0x99, d_99, sizeof(d_99), 0},
};

static inline uint16_t gc9a01_rgb888_to_565(unsigned int rgb888) {
    return (uint16_t)((((rgb888 >> 16) & 0xF8u) << 8) | (((rgb888 >> 8) & 0xFCu) << 3) | ((rgb888 & 0xF8u) >> 3));
}

static inline void gc9a01_tx_begin(void) {
    gc9a01_spi.beginTransaction(gc9a01_spi_settings);
    digitalWrite(XIAO_GC9A01_PIN_CS, LOW);
}

static inline void gc9a01_tx_end(void) {
    digitalWrite(XIAO_GC9A01_PIN_CS, HIGH);
    gc9a01_spi.endTransaction();
}

static inline void gc9a01_tx_cmd(uint8_t cmd) {
    digitalWrite(XIAO_GC9A01_PIN_DC, LOW);
    gc9a01_spi.transfer(cmd);
}

static inline void gc9a01_tx_data(const uint8_t *data, int len) {
    if (!data || len <= 0) return;
    digitalWrite(XIAO_GC9A01_PIN_DC, HIGH);
    gc9a01_spi.writeBytes(data, (uint32_t)len);
}

static void gc9a01_write_cmd(uint8_t cmd) {
    gc9a01_tx_begin();
    gc9a01_tx_cmd(cmd);
    gc9a01_tx_end();
}

static void gc9a01_write_data8(uint8_t data) {
    gc9a01_tx_begin();
    digitalWrite(XIAO_GC9A01_PIN_DC, HIGH);
    gc9a01_spi.transfer(data);
    gc9a01_tx_end();
}

static void gc9a01_write_data16(uint16_t data) {
    uint8_t raw[2];
    raw[0] = (uint8_t)(data >> 8);
    raw[1] = (uint8_t)(data & 0xff);
    gc9a01_tx_begin();
    gc9a01_tx_data(raw, 2);
    gc9a01_tx_end();
}

static void gc9a01_write_data(const uint8_t *data, int len) {
    gc9a01_tx_begin();
    gc9a01_tx_data(data, len);
    gc9a01_tx_end();
}

static void gc9a01_begin_pixels(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1) {
    uint8_t raw[4];
    gc9a01_tx_begin();

    gc9a01_tx_cmd(0x2A);
    raw[0] = (uint8_t)(x0 >> 8);
    raw[1] = (uint8_t)(x0 & 0xff);
    raw[2] = (uint8_t)(x1 >> 8);
    raw[3] = (uint8_t)(x1 & 0xff);
    gc9a01_tx_data(raw, 4);

    gc9a01_tx_cmd(0x2B);
    raw[0] = (uint8_t)(y0 >> 8);
    raw[1] = (uint8_t)(y0 & 0xff);
    raw[2] = (uint8_t)(y1 >> 8);
    raw[3] = (uint8_t)(y1 & 0xff);
    gc9a01_tx_data(raw, 4);

    gc9a01_tx_cmd(0x2C);
    digitalWrite(XIAO_GC9A01_PIN_DC, HIGH);
}

static void gc9a01_end_pixels(void) {
    gc9a01_tx_end();
}

static void gc9a01_init(void) {
    if (gc9a01_ready) return;

    pinMode(XIAO_GC9A01_PIN_CS, OUTPUT);
    pinMode(XIAO_GC9A01_PIN_DC, OUTPUT);
    digitalWrite(XIAO_GC9A01_PIN_CS, HIGH);
    digitalWrite(XIAO_GC9A01_PIN_DC, HIGH);

    if (XIAO_GC9A01_PIN_RST >= 0) {
        pinMode(XIAO_GC9A01_PIN_RST, OUTPUT);
        digitalWrite(XIAO_GC9A01_PIN_RST, HIGH);
        delay(10);
        digitalWrite(XIAO_GC9A01_PIN_RST, LOW);
        delay(10);
        digitalWrite(XIAO_GC9A01_PIN_RST, HIGH);
        delay(120);
    }

    gc9a01_spi.begin(XIAO_GC9A01_PIN_SCK, -1, XIAO_GC9A01_PIN_MOSI, XIAO_GC9A01_PIN_CS);

    gc9a01_write_cmd(0x01);  // SWRESET
    delay(150);
    gc9a01_write_cmd(0x11);  // SLPOUT
    delay(120);
    gc9a01_write_cmd(0x36);  // MADCTL
    gc9a01_write_data8(0x48);  // MX + BGR (fix horizontal mirror)
    gc9a01_write_cmd(0x3A);  // COLMOD
    gc9a01_write_data8(0x55);  // RGB565
    {
        int i;
        for (i = 0; i < (int)(sizeof(gc9a01_init_cmds) / sizeof(gc9a01_init_cmds[0])); i++) {
            gc9a01_write_cmd(gc9a01_init_cmds[i].cmd);
            if (gc9a01_init_cmds[i].size > 0) {
                gc9a01_write_data(gc9a01_init_cmds[i].data, gc9a01_init_cmds[i].size);
            }
            if (gc9a01_init_cmds[i].delay_ms > 0) delay(gc9a01_init_cmds[i].delay_ms);
        }
    }
    gc9a01_write_cmd(0x21);  // INVON
    gc9a01_write_cmd(0x29);  // DISPON
    delay(20);
    gc9a01_ready = 1;
}

static int esp32c3_video_fill_rgb888(unsigned int rgb888) {
    uint16_t c565 = gc9a01_rgb888_to_565(rgb888);
    uint8_t hi = (uint8_t)(c565 >> 8);
    uint8_t lo = (uint8_t)(c565 & 0xff);
    int x;
    int y;

    gc9a01_init();

    for (x = 0; x < XIAO_GC9A01_WIDTH; x++) {
        gc9a01_linebuf[x * 2] = hi;
        gc9a01_linebuf[x * 2 + 1] = lo;
    }

    gc9a01_begin_pixels(0, 0, XIAO_GC9A01_WIDTH - 1, XIAO_GC9A01_HEIGHT - 1);
    for (y = 0; y < XIAO_GC9A01_HEIGHT; y++) {
        gc9a01_spi.writeBytes(gc9a01_linebuf, XIAO_GC9A01_WIDTH * 2);
    }
    gc9a01_end_pixels();

    return 0;
}

static int esp32c3_video_draw_pixel_rgb888(int x, int y, unsigned int rgb888) {
    uint16_t c565;
    uint8_t raw[2];

    if (x < 0 || y < 0 || x >= XIAO_GC9A01_WIDTH || y >= XIAO_GC9A01_HEIGHT) return -1;

    c565 = gc9a01_rgb888_to_565(rgb888);
    raw[0] = (uint8_t)(c565 >> 8);
    raw[1] = (uint8_t)(c565 & 0xff);

    gc9a01_init();
    gc9a01_begin_pixels((uint16_t)x, (uint16_t)y, (uint16_t)x, (uint16_t)y);
    gc9a01_spi.writeBytes(raw, 2);
    gc9a01_end_pixels();
    return 0;
}

static int esp32c3_video_fill_rect_rgb888(int x, int y, int w, int h, unsigned int rgb888) {
    uint16_t c565 = gc9a01_rgb888_to_565(rgb888);
    uint8_t hi = (uint8_t)(c565 >> 8);
    uint8_t lo = (uint8_t)(c565 & 0xff);
    int x0;
    int y0;
    int x1;
    int y1;
    int cw;
    int ch;
    int row;
    int col;

    if (w <= 0 || h <= 0) return -1;

    x0 = x < 0 ? 0 : x;
    y0 = y < 0 ? 0 : y;
    x1 = x + w;
    y1 = y + h;
    if (x1 > XIAO_GC9A01_WIDTH) x1 = XIAO_GC9A01_WIDTH;
    if (y1 > XIAO_GC9A01_HEIGHT) y1 = XIAO_GC9A01_HEIGHT;
    if (x0 >= x1 || y0 >= y1) return -1;

    cw = x1 - x0;
    ch = y1 - y0;

    for (col = 0; col < cw; col++) {
        gc9a01_linebuf[col * 2] = hi;
        gc9a01_linebuf[col * 2 + 1] = lo;
    }

    gc9a01_init();
    gc9a01_begin_pixels((uint16_t)x0, (uint16_t)y0, (uint16_t)(x1 - 1), (uint16_t)(y1 - 1));
    for (row = 0; row < ch; row++) {
        gc9a01_spi.writeBytes(gc9a01_linebuf, cw * 2);
    }
    gc9a01_end_pixels();

    return 0;
}

static int esp32c3_video_blit_rgb888(int x, int y, int w, int h, const unsigned int *pixels, int stride) {
    int src_x0 = 0;
    int src_y0 = 0;
    int row;
    int col;

    if (!pixels || w <= 0 || h <= 0 || stride < w) return -1;

    if (x < 0) {
        src_x0 = -x;
        w += x;
        x = 0;
    }
    if (y < 0) {
        src_y0 = -y;
        h += y;
        y = 0;
    }
    if (x >= XIAO_GC9A01_WIDTH || y >= XIAO_GC9A01_HEIGHT) return -1;

    if (x + w > XIAO_GC9A01_WIDTH) w = XIAO_GC9A01_WIDTH - x;
    if (y + h > XIAO_GC9A01_HEIGHT) h = XIAO_GC9A01_HEIGHT - y;
    if (w <= 0 || h <= 0) return -1;

    gc9a01_init();
    gc9a01_begin_pixels((uint16_t)x, (uint16_t)y, (uint16_t)(x + w - 1), (uint16_t)(y + h - 1));

    for (row = 0; row < h; row++) {
        const unsigned int *src = pixels + (src_y0 + row) * stride + src_x0;
        for (col = 0; col < w; col++) {
            uint16_t c565 = gc9a01_rgb888_to_565(src[col]);
            gc9a01_linebuf[col * 2] = (uint8_t)(c565 >> 8);
            gc9a01_linebuf[col * 2 + 1] = (uint8_t)(c565 & 0xff);
        }
        gc9a01_spi.writeBytes(gc9a01_linebuf, w * 2);
    }

    gc9a01_end_pixels();
    return 0;
}

static int esp32c3_video_size(int *w, int *h) {
    if (!w || !h) return -1;
    *w = 240;
    *h = 240;
    return 0;
}

static int esp32c3_video_set_mode(int w, int h) {
    (void)w;
    (void)h;
    return -1;
}

static const xiao_hal esp32c3_hal = {
    esp32c3_serial_write,
    esp32c3_serial_write,
    esp32c3_input_read,
    esp32c3_wait_ms,
    esp32c3_yield,
    esp32c3_video_fill_rgb888,
    esp32c3_video_draw_pixel_rgb888,
    esp32c3_video_fill_rect_rgb888,
    esp32c3_video_size,
    esp32c3_video_set_mode,
    XIAO_PLATFORM_ESP32,
    esp32c3_video_blit_rgb888,
    0,
};

void setup() {
    Serial.begin(115200);
    unsigned long start = millis();
    while (!Serial && millis() - start < 3000) {
        delay(10);
    }
    esp32c3_serial_connected = Serial ? 1 : 0;
    xiao_start(&esp32c3_hal, &xiao_image);
}

void loop() {
}
