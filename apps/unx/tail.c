#include "xiao.h"

static int streq(const char *a, const char *b) {
    while (*a && *b && *a == *b) { a++; b++; }
    return *a == 0 && *b == 0;
}

static int parse_int(const char *s) {
    int v = 0;
    if (!s || !*s) return -1;
    while (*s) {
        if (*s < '0' || *s > '9') return -1;
        v = v * 10 + (*s - '0');
        s++;
    }
    return v;
}

int xiao_app_entry(xiao_env *env) {
    int argc = xiao_argc(env);
    int i = 1;
    int lines = 10;
    const char *name;
    const char *data;
    xiao_size size;
    xiao_size pos;
    int seen = 0;

    if (argc < 2) {
        xiao_console_print(env, "usage: tail [-n lines] FILE\r\n");
        return 1;
    }

    if (i + 1 < argc && streq(xiao_argv(env, i), "-n")) {
        lines = parse_int(xiao_argv(env, i + 1));
        if (lines < 0) {
            xiao_console_print(env, "tail: invalid line count\r\n");
            return 1;
        }
        i += 2;
    }

    if (i >= argc) {
        xiao_console_print(env, "usage: tail [-n lines] FILE\r\n");
        return 1;
    }

    name = xiao_argv(env, i);
    if (xiao_fs_read(name, &data, &size) != 0) {
        xiao_console_print(env, "tail: not found: ");
        xiao_console_print(env, name);
        xiao_console_print(env, "\r\n");
        return 1;
    }

    pos = size;
    while (pos > 0) {
        pos--;
        if (data[pos] == '\n') {
            seen++;
            if (seen > lines) {
                pos++;
                break;
            }
        }
    }
    if (seen <= lines) pos = 0;

    if (pos < size) xiao_console_write(env, data + pos, size - pos);
    if (size == 0 || data[size - 1] != '\n') xiao_console_print(env, "\r\n");
    return 0;
}
