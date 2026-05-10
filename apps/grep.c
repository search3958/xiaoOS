#include "xiao.h"

static int contains(const char *data, xiao_size len, const char *needle) {
    xiao_size i;
    xiao_size j;
    if (!needle || !needle[0]) return 1;
    for (i = 0; i < len; i++) {
        for (j = 0; needle[j] && i + j < len && data[i + j] == needle[j]; j++) {}
        if (!needle[j]) return 1;
    }
    return 0;
}

static void grep_file(xiao_env *env, const char *pattern, const char *name, int print_name) {
    const char *data = 0;
    xiao_size size = 0;
    xiao_size start = 0;
    xiao_size i;
    if (xiao_file_read(name, &data, &size) != 0) {
        xiao_console_print(env, "grep: not found: ");
        xiao_console_print(env, name);
        xiao_console_print(env, "\r\n");
        return;
    }
    for (i = 0; i <= size; i++) {
        if (i == size || data[i] == '\n') {
            xiao_size len = i - start;
            if (contains(data + start, len, pattern)) {
                if (print_name) {
                    xiao_console_print(env, name);
                    xiao_console_print(env, ":");
                }
                xiao_console_write(env, data + start, len);
                xiao_console_print(env, "\r\n");
            }
            start = i + 1;
        }
    }
}

int xiao_app_entry(xiao_env *env) {
    int i;
    int print_name;
    if (xiao_argc(env) < 3) {
        xiao_console_print(env, "usage: grep PATTERN FILE...\r\n");
        return 1;
    }
    print_name = xiao_argc(env) > 3;
    for (i = 2; i < xiao_argc(env); i++) {
        grep_file(env, xiao_argv(env, 1), xiao_argv(env, i), print_name);
    }
    return 0;
}
