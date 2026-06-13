#include "xiao.h"
#include "multiboot2.h"

// Extern PS/2 driver functions
extern int grub_input_read(void);
extern int grub_get_ctrl(void);
extern int grub_mouse_get(xiao_mouse_state *out);
extern void grub_mouse_init(void);

// Basic GRUB HAL implementation stub
// Kernel will be loaded by GRUB, we need to handle PS/2 keyboard/mouse directly in the kernel

static void grub_serial_write(const char *data, xiao_size len) {
    // To be implemented
}

static void grub_console_write(const char *data, xiao_size len) {
    // To be implemented
}

static void grub_wait_ms(xiao_tick ms) {
    // To be implemented: PIT or HPET timer
}

static void grub_yield(void) {
    __asm__ __volatile__("pause");
}

static int grub_video_fill_rgb888(unsigned int rgb888) {
    return -1;
}

static int grub_video_draw_pixel_rgb888(int x, int y, unsigned int rgb888) {
    return -1;
}

static int grub_video_fill_rect_rgb888(int x, int y, int w, int h, unsigned int rgb888) {
    return -1;
}

static int grub_video_size(int *w, int *h) {
    return -1;
}

static int grub_video_set_mode(int w, int h) {
    return -1;
}

static void grub_mouse_reset(void) {
}

static void grub_mouse_move(int dx, int dy) {
}

static void grub_mouse_set_buttons(int buttons) {
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
    0, // Video blit not implemented yet
    grub_mouse_get,
    grub_mouse_reset,
    grub_mouse_move,
    grub_mouse_set_buttons,
    grub_get_ctrl,
};

void grub_main(void) {
    grub_mouse_init();
    xiao_start(&grub_hal, &xiao_image);
}
