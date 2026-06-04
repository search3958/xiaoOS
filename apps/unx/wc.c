#include "xiao.h"

static void print_uint(xiao_env *env, xiao_size v) {
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

static void count_data(const char *data, xiao_size size, xiao_size *lines, xiao_size *words) {
    xiao_size i;
    int in_word = 0;
    *lines = 0;
    *words = 0;
    for (i = 0; i < size; i++) {
        char c = data[i];
        if (c == '\n') (*lines)++;
        if (c == ' ' || c == '\t' || c == '\r' || c == '\n') {
            in_word = 0;
        } else if (!in_word) {
            (*words)++;
            in_word = 1;
        }
    }
}

static void print_counts(xiao_env *env, xiao_size l, xiao_size w, xiao_size b, const char *name) {
    print_uint(env, l);
    xiao_console_print(env, " ");
    print_uint(env, w);
    xiao_console_print(env, " ");
    print_uint(env, b);
    xiao_console_print(env, " ");
    xiao_console_print(env, name);
    xiao_console_print(env, "\r\n");
}

int xiao_app_entry(xiao_env *env) {
    int i;
    xiao_size total_l = 0, total_w = 0, total_b = 0;
    if (xiao_argc(env) < 2) {
        xiao_console_print(env, "usage: wc FILE...\r\n");
        return 1;
    }
    for (i = 1; i < xiao_argc(env); i++) {
        const char *data;
        xiao_size size;
        xiao_size l, w;
        const char *name = xiao_argv(env, i);
        if (xiao_fs_read(name, &data, &size) != 0) {
            xiao_console_print(env, "wc: not found: ");
            xiao_console_print(env, name);
            xiao_console_print(env, "\r\n");
            continue;
        }
        count_data(data, size, &l, &w);
        print_counts(env, l, w, size, name);
        total_l += l;
        total_w += w;
        total_b += size;
    }
    if (xiao_argc(env) > 2) {
        print_counts(env, total_l, total_w, total_b, "total");
    }
    return 0;
}
