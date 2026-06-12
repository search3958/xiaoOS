#include "multiboot2.h"
#include "xiao.h"

typedef unsigned char u8;
typedef unsigned short u16;
typedef unsigned int u32;
typedef unsigned long long u64;

#define COM1 0x3f8

static struct multiboot_tag_framebuffer *fb_tag = 0;
static int mouse_x = 0;
static int mouse_y = 0;
static int mouse_btns = 0;
static int mouse_cycle = 0;
static u8 mouse_bytes[3];
static int mouse_init_done = 0;
static int ctrl_pressed = 0;

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

static void grub_serial_write(const char *data, xiao_size len) {
    xiao_size i;
    for (i = 0; i < len; i++) serial_putc(data[i]);
}

static void grub_console_write(const char *data, xiao_size len) {
    grub_serial_write(data, len);
}

static int grub_input_read(void) {
    static unsigned char kbd_us[128] = {
        0,  27, '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '=', '\b',
        '\t', 'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p', '[', ']', '\n',
        0, 'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', ';', '\'', '`', 0, '\\',
        'z', 'x', 'c', 'v', 'b', 'n', 'm', ',', '.', '/', 0, '*', 0, ' '
    };

    if (inb(0x64) & 1) {
        u8 b = inb(0x60);
        if (b == 0x1d) { ctrl_pressed = 1; return -1; }
        if (b == 0x9d) { ctrl_pressed = 0; return -1; }
        if (b < 128 && kbd_us[b]) return (int)kbd_us[b];
    }
    return -1;
}

static void mouse_wait(u8 type) {
    u32 timeout = 100000;
    if (type == 0) {
        while (timeout--) {
            if ((inb(0x64) & 1) == 1) return;
        }
    } else {
        while (timeout--) {
            if ((inb(0x64) & 2) == 0) return;
        }
    }
}

static void mouse_write(u8 a) {
    mouse_wait(1);
    outb(0x64, 0xD4);
    mouse_wait(1);
    outb(0x60, a);
}

static u8 mouse_read(void) {
    mouse_wait(0);
    return inb(0x60);
}

static void mouse_init(void) {
    u8 status;
    mouse_wait(1);
    outb(0x64, 0xAD);
    mouse_wait(1);
    outb(0x64, 0xA7);
    while (inb(0x64) & 1) inb(0x60);
    mouse_wait(1);
    outb(0x64, 0x20);
    mouse_wait(0);
    status = inb(0x60) | 2;
    mouse_wait(1);
    outb(0x64, 0x60);
    mouse_wait(1);
    outb(0x60, status);
    mouse_wait(1);
    outb(0x64, 0xA8);
    mouse_write(0xF6);
    mouse_read();
    mouse_write(0xF4);
    mouse_read();
    mouse_wait(1);
    outb(0x64, 0xAE);
    mouse_init_done = 1;
    mouse_cycle = 0;
}

static void mouse_poll(void) {
    if (!mouse_init_done) mouse_init();
    int limit = 20;
    while (limit-- > 0) {
        u8 status = inb(0x64);
        if (!(status & 1)) break;
        u8 b = inb(0x60);
        if (!(status & 0x20)) continue;
        if (mouse_cycle == 0 && !(b & 0x08)) continue;
        mouse_bytes[mouse_cycle++] = b;
        if (mouse_cycle == 3) {
            mouse_cycle = 0;
            if (mouse_bytes[0] & 0x80 || mouse_bytes[0] & 0x40) continue;
            int dx = (int)mouse_bytes[1];
            int dy = (int)mouse_bytes[2];
            if (mouse_bytes[0] & 0x10) dx -= 256;
            if (mouse_bytes[0] & 0x20) dy -= 256;
            mouse_x += dx;
            mouse_y -= dy;
            int w = 1280, h = 720;
            if (fb_tag) {
                w = fb_tag->common.framebuffer_width;
                h = fb_tag->common.framebuffer_height;
            }
            if (mouse_x < 0) mouse_x = 0;
            if (mouse_y < 0) mouse_y = 0;
            if (mouse_x >= w) mouse_x = w - 1;
            if (mouse_y >= h) mouse_y = h - 1;
            mouse_btns = mouse_bytes[0] & 0x07;
        }
    }
}

static int grub_mouse_get(xiao_mouse_state *out) {
    mouse_poll();
    if (out) {
        out->x = mouse_x;
        out->y = mouse_y;
        out->buttons = mouse_btns;
    }
    return 0;
}

static void grub_wait_ms(xiao_tick ms) {
    volatile u64 i;
    while (ms--) {
        for (i = 0; i < 1000000; i++) __asm__ __volatile__("pause");
    }
}

static void grub_yield(void) {
    __asm__ __volatile__("pause");
}

static int grub_video_draw_pixel_rgb888(int x, int y, unsigned int rgb888) {
    if (!fb_tag) return -1;
    if (x < 0 || y < 0 || x >= (int)fb_tag->common.framebuffer_width || y >= (int)fb_tag->common.framebuffer_height) return -1;
    if (fb_tag->common.framebuffer_bpp == 32) {
        u32 *ptr = (u32 *)(uintptr_t)fb_tag->common.framebuffer_addr;
        ptr[y * (fb_tag->common.framebuffer_pitch / 4) + x] = rgb888;
        return 0;
    }
    return -1;
}

static int grub_video_fill_rect_rgb888(int x, int y, int w, int h, unsigned int rgb888) {
    int ix, iy;
    for (iy = 0; iy < h; iy++) {
        for (ix = 0; ix < w; ix++) {
            grub_video_draw_pixel_rgb888(x + ix, y + iy, rgb888);
        }
    }
    return 0;
}

static int grub_video_fill_rgb888(unsigned int rgb888) {
    if (!fb_tag) return -1;
    return grub_video_fill_rect_rgb888(0, 0, fb_tag->common.framebuffer_width, fb_tag->common.framebuffer_height, rgb888);
}

static int grub_video_size(int *w, int *h) {
    if (!fb_tag) {
        *w = 1280; *h = 720;
        return -1;
    }
    *w = fb_tag->common.framebuffer_width;
    *h = fb_tag->common.framebuffer_height;
    return 0;
}

static int grub_video_set_mode(int w, int h) {
    (void)w; (void)h;
    return 0;
}

static int grub_video_blit_rgb888(int x, int y, int w, int h, const unsigned int *pixels, int stride) {
    int ix, iy;
    for (iy = 0; iy < h; iy++) {
        for (ix = 0; ix < w; ix++) {
            grub_video_draw_pixel_rgb888(x + ix, y + iy, pixels[iy * stride + ix]);
        }
    }
    return 0;
}

static void grub_mouse_reset(void) {
    mouse_init();
}

static void grub_mouse_move(int dx, int dy) {
    mouse_x += dx;
    mouse_y -= dy;
}

static void grub_mouse_set_buttons(int buttons) {
    mouse_btns = buttons;
}

static int grub_get_ctrl(void) {
    return ctrl_pressed;
}

static const xiao_hal grub_hal = {
    grub_serial_write,
    grub_console_write,
    grub_input_read,
    grub_wait_ms,
    grub_yield,
    grub_video_fill_rgb888,
    grub_video_draw_pixel_rgb888,
    grub_video_fill_rect_rgb888,
    grub_video_size,
    grub_video_set_mode,
    XIAO_PLATFORM_PC,
    grub_video_blit_rgb888,
    grub_mouse_get,
    grub_mouse_reset,
    grub_mouse_move,
    grub_mouse_set_buttons,
    grub_get_ctrl,
};

void grub_main(u32 magic, u32 addr) {
    serial_init();
    grub_serial_write("GRUB: xiaoOS loading...\n", 25);

    if (magic != MULTIBOOT2_BOOTLOADER_MAGIC) {
        grub_serial_write("GRUB: ERR: Invalid Multiboot2 magic\n", 36);
        while(1);
    }

    struct multiboot_tag *tag;
    for (tag = (struct multiboot_tag *)(uintptr_t)(addr + 8);
         tag->type != MULTIBOOT_TAG_TYPE_END;
         tag = (struct multiboot_tag *)((u8 *)tag + ((tag->size + 7) & ~7))) {
        if (tag->type == MULTIBOOT_TAG_TYPE_FRAMEBUFFER) {
            fb_tag = (struct multiboot_tag_framebuffer *)tag;
        }
    }

    if (!fb_tag) {
        grub_serial_write("GRUB: WRN: Framebuffer not found\n", 33);
    }

    grub_serial_write("GRUB: xiao_start calling...\n", 28);
    xiao_start(&grub_hal, &xiao_image);
    while (1) __asm__ __volatile__("hlt");
}
