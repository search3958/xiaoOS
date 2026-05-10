#include "xiao.h"

typedef unsigned char u8;
typedef unsigned short u16;
typedef unsigned int u32;

#define COM1 0x3f8
#define VGA_W 80
#define VGA_H 25

static u16 *const vga = (u16 *)0xb8000;
static u32 row;
static u32 col;
static int esc_state;

static void console_putc(char c);
static void console_clear(void);

static void outb(u16 port, u8 value) {
    __asm__ __volatile__("outb %0, %1" : : "a"(value), "Nd"(port));
}

static u8 inb(u16 port) {
    u8 value;
    __asm__ __volatile__("inb %1, %0" : "=a"(value) : "Nd"(port));
    return value;
}

static void serial_init(void) {
    outb(COM1 + 1, 0x00);
    outb(COM1 + 3, 0x80);
    outb(COM1 + 0, 0x01);
    outb(COM1 + 1, 0x00);
    outb(COM1 + 3, 0x03);
    outb(COM1 + 2, 0xc7);
    outb(COM1 + 4, 0x0b);
}

static void serial_putc(char c) {
    while ((inb(COM1 + 5) & 0x20) == 0) {}
    outb(COM1, (u8)c);
}

static void bios_serial_write(const char *data, xiao_size len) {
    xiao_size i;
    for (i = 0; i < len; i++) {
        serial_putc(data[i]);
        console_putc(data[i]);
    }
}

static void scroll_if_needed(void) {
    u32 i;
    if (row < VGA_H) return;
    for (i = 0; i < (VGA_H - 1) * VGA_W; i++) vga[i] = vga[i + VGA_W];
    for (i = (VGA_H - 1) * VGA_W; i < VGA_H * VGA_W; i++) vga[i] = 0x0720;
    row = VGA_H - 1;
}

static void console_clear(void) {
    u32 i;
    for (i = 0; i < VGA_W * VGA_H; i++) vga[i] = 0x0720;
    row = 0;
    col = 0;
}

static void console_putc(char c) {
    if (esc_state == 1) {
        if (c == '[') {
            esc_state = 2;
            return;
        }
        esc_state = 0;
    } else if (esc_state == 2) {
        if (c == 'H') {
            row = 0;
            col = 0;
            esc_state = 0;
            return;
        }
        if (c == '2') {
            esc_state = 3;
            return;
        }
        esc_state = 0;
    } else if (esc_state == 3) {
        if (c == 'J') {
            console_clear();
            esc_state = 0;
            return;
        }
        esc_state = 0;
    }
    if ((u8)c == 0x1b) {
        esc_state = 1;
        return;
    }

    if (c == '\r') return;
    if (c == '\n') {
        row++;
        col = 0;
        scroll_if_needed();
        return;
    }
    vga[row * VGA_W + col] = (u16)(0x0700 | (u8)c);
    col++;
    if (col >= VGA_W) {
        col = 0;
        row++;
        scroll_if_needed();
    }
}

static void bios_console_write(const char *data, xiao_size len) {
    xiao_size i;
    for (i = 0; i < len; i++) {
        console_putc(data[i]);
        serial_putc(data[i]);
    }
}

static int bios_input_read(void) {
    if ((inb(COM1 + 5) & 0x01) == 0) return -1;
    return (int)inb(COM1);
}

static void bios_wait_ms(xiao_tick ms) {
    volatile unsigned long i;
    while (ms--) {
        for (i = 0; i < 40000; i++) {}
    }
}

static void bios_yield(void) {
    __asm__ __volatile__("pause");
}

static u8 bios_color_bg(unsigned int rgb888) {
    u8 r = (u8)((rgb888 >> 16) & 0xff);
    u8 g = (u8)((rgb888 >> 8) & 0xff);
    u8 b = (u8)(rgb888 & 0xff);
    u8 bg = 0;
    if (r >= 96) bg |= 0x4;
    if (g >= 96) bg |= 0x2;
    if (b >= 96) bg |= 0x1;
    return bg;
}

static int bios_video_draw_pixel_rgb888(int x, int y, unsigned int rgb888) {
    u8 bg;
    if (x < 0 || y < 0 || x >= VGA_W || y >= VGA_H) return -1;
    bg = bios_color_bg(rgb888);
    vga[y * VGA_W + x] = (u16)(((u16)bg << 12) | 0x0020);
    return 0;
}

static int bios_video_fill_rect_rgb888(int x, int y, int w, int h, unsigned int rgb888) {
    int xx, yy;
    if (w <= 0 || h <= 0) return -1;
    for (yy = 0; yy < h; yy++) {
        for (xx = 0; xx < w; xx++) {
            bios_video_draw_pixel_rgb888(x + xx, y + yy, rgb888);
        }
    }
    return 0;
}

static int bios_video_fill_rgb888(unsigned int rgb888) {
    u32 i;
    u8 bg = bios_color_bg(rgb888);
    for (i = 0; i < VGA_W * VGA_H; i++) vga[i] = (u16)(((u16)bg << 12) | 0x0020);
    row = 0;
    col = 0;
    return 0;
}

static int bios_video_size(int *w, int *h) {
    if (!w || !h) return -1;
    *w = VGA_W;
    *h = VGA_H;
    return 0;
}

static const xiao_hal bios_hal = {
    bios_serial_write,
    bios_console_write,
    bios_input_read,
    bios_wait_ms,
    bios_yield,
    bios_video_fill_rgb888,
    bios_video_draw_pixel_rgb888,
    bios_video_fill_rect_rgb888,
    bios_video_size,
    XIAO_PLATFORM_PC,
};

void xiao_bios_main(void) {
    serial_init();
    xiao_start(&bios_hal, &xiao_image);
}
