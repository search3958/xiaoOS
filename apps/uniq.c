#include "xiao.h"

#define MAX_LINE 128

static int streq(const char *a, const char *b) {
    while (*a && *b && *a == *b) { a++; b++; }
    return *a == 0 && *b == 0;
}

int xiao_app_entry(xiao_env *env) {
    const char *data;
    xiao_size size;
    xiao_size i = 0;
    char prev[MAX_LINE];
    int has_prev = 0;

    if (xiao_argc(env) != 2) {
        xiao_console_print(env, "usage: uniq FILE\r\n");
        return 1;
    }
    if (xiao_fs_read(xiao_argv(env, 1), &data, &size) != 0) {
        xiao_console_print(env, "uniq: not found: ");
        xiao_console_print(env, xiao_argv(env, 1));
        xiao_console_print(env, "\r\n");
        return 1;
    }

    while (i <= size) {
        char line[MAX_LINE];
        int p = 0;
        while (i < size && data[i] != '\n') {
            if (data[i] != '\r' && p + 1 < MAX_LINE) line[p++] = data[i];
            i++;
        }
        line[p] = 0;

        if (!has_prev || !streq(prev, line)) {
            xiao_console_print(env, line);
            xiao_console_print(env, "\r\n");
            {
                int k = 0;
                while (line[k] && k + 1 < MAX_LINE) { prev[k] = line[k]; k++; }
                prev[k] = 0;
            }
            has_prev = 1;
        }

        if (i >= size) break;
        i++;
    }
    return 0;
}
