#include "xiao.h"

int xiao_app_entry(xiao_env *env) {
    if (xiao_video_fill_rgb888(env, 0xAA0000u) != 0) {
        xiao_console_print(env, "red: video fill is not supported on this target\r\n");
        return 1;
    }
    return 0;
}
