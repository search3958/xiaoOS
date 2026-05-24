#include "xiao.h"

static int streq(const char *a, const char *b) {
    while (*a && *b && *a == *b) { a++; b++; }
    return *a == 0 && *b == 0;
}

static int parse_int(const char *s, int *out) {
    int sign = 1;
    int v = 0;
    if (!s || !*s || !out) return -1;
    if (*s == '-') { sign = -1; s++; }
    if (!*s) return -1;
    while (*s) {
        if (*s < '0' || *s > '9') return -1;
        v = v * 10 + (*s - '0');
        s++;
    }
    *out = sign * v;
    return 0;
}

static int hex_nibble(char c) {
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'a' && c <= 'f') return 10 + (c - 'a');
    if (c >= 'A' && c <= 'F') return 10 + (c - 'A');
    return -1;
}

static int parse_color(const char *s, unsigned int *out) {
    int i;
    unsigned int v = 0;
    if (!s || !out) return -1;
    if (s[0] == '#') s++;
    if (s[0] == '0' && (s[1] == 'x' || s[1] == 'X')) s += 2;
    for (i = 0; i < 6; i++) {
        int n = hex_nibble(s[i]);
        if (n < 0) return -1;
        v = (v << 4) | (unsigned int)n;
    }
    if (s[6] != 0) return -1;
    *out = v;
    return 0;
}

int xiao_app_entry(xiao_env *env) {
    int x, y, w, h;
    unsigned int c;

    if (xiao_argc(env) < 2) {
        xiao_console_print(env, "usage: grap drawpixel X Y COLOR\r\n");
        xiao_console_print(env, "   or: grap drawrect X Y W H COLOR\r\n");
        return 1;
    }

    if (streq(xiao_argv(env, 1), "drawpixel")) {
        if (xiao_argc(env) != 5 ||
            parse_int(xiao_argv(env, 2), &x) != 0 ||
            parse_int(xiao_argv(env, 3), &y) != 0 ||
            parse_color(xiao_argv(env, 4), &c) != 0) {
            xiao_console_print(env, "usage: grap drawpixel X Y COLOR\r\n");
            return 1;
        }
        if (xiao_video_draw_pixel_rgb888(env, x, y, c) != 0) {
            xiao_console_print(env, "grap: drawpixel failed\r\n");
            return 1;
        }
        return 0;
    }

    if (streq(xiao_argv(env, 1), "drawrect")) {
        if (xiao_argc(env) != 7 ||
            parse_int(xiao_argv(env, 2), &x) != 0 ||
            parse_int(xiao_argv(env, 3), &y) != 0 ||
            parse_int(xiao_argv(env, 4), &w) != 0 ||
            parse_int(xiao_argv(env, 5), &h) != 0 ||
            parse_color(xiao_argv(env, 6), &c) != 0) {
            xiao_console_print(env, "usage: grap drawrect X Y W H COLOR\r\n");
            return 1;
        }
        if (xiao_video_fill_rect_rgb888(env, x, y, w, h, c) != 0) {
            xiao_console_print(env, "grap: drawrect failed\r\n");
            return 1;
        }
        return 0;
    }

    xiao_console_print(env, "grap: unknown command\r\n");
    return 1;
}
