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

    while (1) {
        int key;

        xiao_video_fill_rect_rgb888(
            env,
            0,
            0,
            sw,
            sh,
            CLUTCHPAD_BG
        );

        key = xiao_input_read(env);

        if (key >= 0) {
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