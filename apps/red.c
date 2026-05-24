#include "xiao.h"

int xiao_app_entry(xiao_env *env) {
    int sw = 0;
    int sh = 0;
    int step;
    int total_steps = 80;

    if (xiao_video_size(env, &sw, &sh) != 0) {
        xiao_console_print(env, "red: video size is not available\r\n");
        return 1;
    }
    if (xiao_video_fill_rect_rgb888(env, 0, 0, sw, sh, 0xAA0000u) != 0) {
        xiao_console_print(env, "red: background draw failed\r\n");
        return 1;
    }

    for (step = 1; step <= total_steps; step++) {
        int size = step;
        int x = sw / 2 - size / 2;
        int y = sh / 2 - size / 2;
        xiao_video_fill_rect_rgb888(env, x, y, size, size, 0xFFCB52u);
        xiao_wait(env, 25);
    }
    return 0;
}
