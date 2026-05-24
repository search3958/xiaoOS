#include "xiao.h"

int xiao_app_entry(xiao_env *env) {
    int i;
    if (xiao_argc(env) < 2) {
        xiao_console_print(env, "usage: rm PATH...\r\n");
        return 1;
    }
    for (i = 1; i < xiao_argc(env); i++) {
        const char *path = xiao_argv(env, i);
        if (xiao_fs_remove(path) != 0) {
            xiao_console_print(env, "rm: failed: ");
            xiao_console_print(env, path);
            xiao_console_print(env, "\r\n");
        }
    }
    return 0;
}
