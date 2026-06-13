#include "multiboot2.h"

// Basic GRUB HAL implementation stub
// Kernel will be loaded by GRUB, we need to handle PS/2 keyboard/mouse directly in the kernel

static void grub_serial_write(const char *data, xiao_size len) {
    // To be implemented
}

static void grub_console_write(const char *data, xiao_size len) {
    // To be implemented
}

static int grub_input_read(void) {
    // To be implemented: PS/2 Keyboard driver
    return -1;
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

static int grub_mouse_get(xiao_mouse_state *out) {
    // To be implemented: PS/2 Mouse driver
    return -1;
}

static void grub_mouse_reset(void) {
}

static void grub_mouse_move(int dx, int dy) {
}

static void grub_mouse_set_buttons(int buttons) {
}

static int grub_get_ctrl(void) {
    return 0;
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
    // Initialize things and call xiao_start
    xiao_start(&grub_hal, &xiao_image);
}
