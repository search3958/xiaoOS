#include "limine.h"
#include "xiao.h"

#define COM1 0x3f8

__attribute__((used, section(".limine_requests"), aligned(8)))
static volatile LIMINE_BASE_REVISION(1);

__attribute__((used, section(".limine_requests"), aligned(8)))
static volatile struct limine_framebuffer_request framebuffer_request = {
    .id = LIMINE_FRAMEBUFFER_REQUEST,
    .revision = 0
};

static void outb(unsigned short port, unsigned char val) {
    asm volatile ("outb %0, %1" : : "a"(val), "Nd"(port));
}

static unsigned char inb(unsigned short port) {
    unsigned char ret;
    asm volatile ("inb %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
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
    while ((inb(COM1 + 5) & 0x20) == 0);
    outb(COM1, c);
}

static void limine_serial_write(const char *data, xiao_size len) {
    xiao_size i;
    for (i = 0; i < len; i++) serial_putc(data[i]);
}

static void limine_console_write(const char *data, xiao_size len) {
    // For now, just write to serial. xiaoOS apps often use serial for console.
    limine_serial_write(data, len);
}

static int limine_input_read(void) {
    // Basic PS/2 keyboard poller (set 1)
    if ((inb(0x64) & 1) == 0) return -1;
    unsigned char scancode = inb(0x60);
    if (scancode & 0x80) return -1; // Ignore release

    // Very minimal scancode to ASCII map
    static const char map[] = {
        0, 0, '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '=', '\b',
        '\t', 'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p', '[', ']', '\n',
        0, 'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', ';', '\'', '`', 0, '\\',
        'z', 'x', 'c', 'v', 'b', 'n', 'm', ',', '.', '/', 0, '*', 0, ' '
    };
    if (scancode < sizeof(map)) return map[scancode];
    return -1;
}

static void limine_wait_ms(xiao_tick ms) {
    // Very rough busy wait
    volatile unsigned long i;
    while (ms--) {
        for (i = 0; i < 1000000; i++) asm volatile("nop");
    }
}

static void limine_yield(void) {
    asm volatile("pause");
}

static int limine_video_size(int *w, int *h) {
    if (framebuffer_request.response == 0 || framebuffer_request.response->framebuffer_count == 0) return -1;
    struct limine_framebuffer *fb = framebuffer_request.response->framebuffers[0];
    *w = (int)fb->width;
    *h = (int)fb->height;
    return 0;
}

static int limine_video_draw_pixel_rgb888(int x, int y, unsigned int rgb888) {
    if (framebuffer_request.response == 0 || framebuffer_request.response->framebuffer_count == 0) return -1;
    struct limine_framebuffer *fb = framebuffer_request.response->framebuffers[0];
    if (x < 0 || y < 0 || x >= (int)fb->width || y >= (int)fb->height) return -1;
    
    // Support 32-bit RGB
    if (fb->bpp == 32) {
        unsigned int *ptr = (unsigned int *)fb->address;
        ptr[y * (fb->pitch / 4) + x] = rgb888;
        return 0;
    }
    return -1;
}

static int limine_video_fill_rect_rgb888(int x, int y, int w, int h, unsigned int rgb888) {
    int iy, ix;
    for (iy = 0; iy < h; iy++) {
        for (ix = 0; ix < w; ix++) {
            limine_video_draw_pixel_rgb888(x + ix, y + iy, rgb888);
        }
    }
    return 0;
}

static int limine_video_fill_rgb888(unsigned int rgb888) {
    int w, h;
    limine_video_size(&w, &h);
    return limine_video_fill_rect_rgb888(0, 0, w, h, rgb888);
}

static int limine_video_set_mode(int w, int h) {
    // Limine sets the mode before booting, usually we don't change it here
    (void)w; (void)h;
    return 0;
}

static int limine_video_blit_rgb888(int x, int y, int w, int h, const unsigned int *pixels, int stride) {
    int iy, ix;
    for (iy = 0; iy < h; iy++) {
        for (ix = 0; ix < w; ix++) {
            limine_video_draw_pixel_rgb888(x + ix, y + iy, pixels[iy * stride + ix]);
        }
    }
    return 0;
}

static int limine_mouse_get(xiao_mouse_state *out) {
    (void)out;
    return -1;
}

static void limine_mouse_reset(void) {}
static void limine_mouse_move(int dx, int dy) { (void)dx; (void)dy; }
static void limine_mouse_set_buttons(int buttons) { (void)buttons; }
static int limine_get_ctrl(void) { return 0; }

static const xiao_hal limine_hal = {
    limine_serial_write,
    limine_console_write,
    limine_input_read,
    limine_wait_ms,
    limine_yield,
    limine_video_fill_rgb888,
    limine_video_draw_pixel_rgb888,
    limine_video_fill_rect_rgb888,
    limine_video_size,
    limine_video_set_mode,
    XIAO_PLATFORM_PC,
    limine_video_blit_rgb888,
    limine_mouse_get,
    limine_mouse_reset,
    limine_mouse_move,
    limine_mouse_set_buttons,
    limine_get_ctrl,
};

void _start(void) {
    serial_init();
    xiao_start(&limine_hal, &xiao_image);
    while (1) asm volatile("hlt");
}
