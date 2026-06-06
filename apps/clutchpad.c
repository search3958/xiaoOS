#include "xiao.h"

#define CLUTCHPAD_BG 0xAA0000u

int xiao_app_entry(xiao_env *env) {
    int sw = 0;
    int sh = 0;
    int key_count = 0;

    if (!env) {
        return 1;
    }

    if (xiao_video_size(env, &sw, &sh) != 0) {
        xiao_console_print(env, "clutchpad: video unavailable\r\n");
        return 1;
    }

    xiao_console_print(env, "clutchpad: started\r\n");
    xiao_exec_line("mode gui");
    xiao_mouse_reset(env);

    while (1) {
        int key;
        xiao_mouse_state mouse;

        xiao_video_fill_rect_rgb888(
            env,
            0,
            0,
            sw,
            sh,
            CLUTCHPAD_BG
        );

        if (xiao_mouse_get(env, &mouse) == 0) {
            // Adaptive cursor size: bigger on high-res screens
            int cursor_size = (sw > 320) ? 12 : 8;
            unsigned int cursor_color = (mouse.buttons != 0) ? 0x000000u : 0xFFFFFFu;
            xiao_video_fill_rect_rgb888(
                env, 
                mouse.x - cursor_size / 2, 
                mouse.y - cursor_size / 2, 
                cursor_size, 
                cursor_size, 
                cursor_color
            );
        }

        key = xiao_input_read(env);

        if (key == 'r') {
            xiao_mouse_reset(env);
            xiao_console_print(env, "clutchpad: mouse reset requested\r\n");
        } else if (key >= 0) {
            key_count++;

            xiao_console_print(
                env,
                "clutchpad: key detected\r\n"
            );

            if (key_count >= 3) {
                xiao_console_print(
                    env,
                    "clutchpad: exiting\r\n"
                );
                break;
            }
        }

        xiao_wait(env, 16);
    }

    xiao_exec_line("mode cli");
    return 0;
}
