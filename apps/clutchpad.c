#include "xiao.h"

#define CLUTCHPAD_BG 0xAA0000u

static int streq(const char *a, const char *b) {
    while (*a && *b && *a == *b) { a++; b++; }
    return *a == 0 && *b == 0;
}

static void print_uint(xiao_env *env, int val) {
    char buf[16];
    int i = 14;
    buf[15] = '\0';
    if (val < 0) {
        xiao_console_print(env, "-");
        val = -val;
    }
    if (val == 0) buf[--i] = '0';
    else {
        while (val > 0) {
            buf[--i] = (char)((val % 10) + '0');
            val /= 10;
        }
    }
    xiao_console_print(env, &buf[i]);
}

int xiao_app_entry(xiao_env *env) {
    if (!env) return 1;

    int debug_mode = 0;
    int argc = xiao_argc(env);
    if (argc > 1) {
        if (streq(xiao_argv(env, 1), "mouce")) {
            debug_mode = 1;
        }
    }

    if (debug_mode) {
        xiao_console_print(env, "clutchpad: mouse debug mode (text only)\r\n");
        xiao_mouse_reset(env);
        while (1) {
            xiao_mouse_state mouse;
            int key = xiao_input_read(env);
            if (key == 'q' || key == 'x') break;

            if (xiao_mouse_get(env, &mouse) == 0) {
                static int last_x = -1, last_y = -1, last_btns = -1;
                if (mouse.x != last_x || mouse.y != last_y || mouse.buttons != last_btns) {
                    xiao_console_print(env, "Mouse: X=");
                    print_uint(env, mouse.x);
                    xiao_console_print(env, ", Y=");
                    print_uint(env, mouse.y);
                    xiao_console_print(env, ", Buttons=");
                    if (mouse.buttons & 1) xiao_console_print(env, "L");
                    if (mouse.buttons & 2) xiao_console_print(env, "R");
                    if (!(mouse.buttons & 1) && !(mouse.buttons & 2)) xiao_console_print(env, "None");
                    xiao_console_print(env, "\r\n");
                    last_x = mouse.x; last_y = mouse.y; last_btns = mouse.buttons;
                }
            }
            xiao_wait(env, 100);
        }
        return 0;
    }

    int sw = 0, sh = 0;
    if (xiao_video_size(env, &sw, &sh) != 0) {
        xiao_console_print(env, "clutchpad: video unavailable\r\n");
        return 1;
    }
    xiao_exec_line("mode gui");
    xiao_mouse_reset(env);

    while (1) {
        xiao_mouse_state mouse;
        int key = xiao_input_read(env);
        if (key == 'q' || key == 'x') break;

        xiao_video_fill_rect_rgb888(env, 0, 0, sw, sh, CLUTCHPAD_BG);
        if (xiao_mouse_get(env, &mouse) == 0) {
            int cursor_size = (sw > 320) ? 12 : 8;
            unsigned int color = (mouse.buttons != 0) ? 0x000000u : 0xFFFFFFu;
            xiao_video_fill_rect_rgb888(env, mouse.x - cursor_size/2, mouse.y - cursor_size/2, cursor_size, cursor_size, color);
        }
        xiao_wait(env, 30);
    }
    xiao_exec_line("mode cli");
    return 0;
}
