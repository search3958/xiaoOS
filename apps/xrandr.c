#include "xiao.h"

static int streq(const char *a, const char *b) {
    while (*a && *b && *a == *b) { a++; b++; }
    return *a == 0 && *b == 0;
}

static void print_uint(xiao_env *env, unsigned int v) {
    char buf[24];
    int i = 23;
    buf[i--] = 0;
    if (v == 0) {
        xiao_console_print(env, "0");
        return;
    }
    while (v > 0 && i >= 0) {
        buf[i--] = (char)('0' + (v % 10));
        v /= 10;
    }
    xiao_console_print(env, &buf[i + 1]);
}

static int parse_mode(const char *s, int *w, int *h) {
    int vw = 0;
    int vh = 0;
    if (!s || !w || !h) return -1;
    while (*s >= '0' && *s <= '9') {
        vw = vw * 10 + (*s - '0');
        s++;
    }
    if (*s != 'x' && *s != 'X') return -1;
    s++;
    while (*s >= '0' && *s <= '9') {
        vh = vh * 10 + (*s - '0');
        s++;
    }
    if (*s != 0 || vw <= 0 || vh <= 0) return -1;
    *w = vw;
    *h = vh;
    return 0;
}

static void print_current(xiao_env *env) {
    int w = 0;
    int h = 0;
    if (xiao_video_size(env, &w, &h) != 0) {
        xiao_console_print(env, "xrandr: current mode unavailable\r\n");
        return;
    }
    xiao_console_print(env, "Screen 0: current ");
    print_uint(env, (unsigned int)w);
    xiao_console_print(env, " x ");
    print_uint(env, (unsigned int)h);
    xiao_console_print(env, "\r\n");
}

int xiao_app_entry(xiao_env *env) {
    if (xiao_platform(env) == XIAO_PLATFORM_ESP32) {
        xiao_console_print(env, "xrandr: not supported on esp32\r\n");
        return 1;
    }

    if (xiao_argc(env) == 1 || (xiao_argc(env) == 2 && streq(xiao_argv(env, 1), "-q"))) {
        print_current(env);
        return 0;
    }

    if (xiao_argc(env) == 3 && streq(xiao_argv(env, 1), "-s")) {
        int w = 0;
        int h = 0;
        if (parse_mode(xiao_argv(env, 2), &w, &h) != 0) {
            xiao_console_print(env, "xrandr: invalid mode format, use WIDTHxHEIGHT\r\n");
            return 1;
        }
        if (xiao_video_set_mode(env, w, h) != 0) {
            xiao_console_print(env, "xrandr: failed to set mode\r\n");
            return 1;
        }
        print_current(env);
        return 0;
    }

    xiao_console_print(env, "usage: xrandr [-q]\r\n");
    xiao_console_print(env, "   or: xrandr -s WIDTHxHEIGHT\r\n");
    return 1;
}
