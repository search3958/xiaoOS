#include "xiao.h"

static void usage(xiao_env *env) {
    xiao_console_print(env, "usage: cat FILENAME...\r\n");
}

int xiao_app_entry(xiao_env *env) {
    int i;
    if (xiao_argc(env) < 2) {
        usage(env);
        return 1;
    }

    for (i = 1; i < xiao_argc(env); i++) {
        const char *data = 0;
        xiao_size size = 0;
        const char *name = xiao_argv(env, i);
        
        if (xiao_file_read(name, &data, &size) != 0) {
            xiao_console_print(env, "cat: not found: ");
            xiao_console_print(env, name);
            xiao_console_print(env, "\r\n");
            continue;
        }

        xiao_console_write(env, data, size);

        if (size == 0 || data[size - 1] != '\n') {
            xiao_console_print(env, "\r\n");
        }
    }
    return 0;
}