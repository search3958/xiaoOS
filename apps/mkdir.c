#include "xiao.h"

int xiao_app_entry(xiao_env *env) {
    int i;
    if (xiao_argc(env) < 2) {
        xiao_console_print(env, "usage: mkdir DIR...\r\n");
        return 1;
    }
    for (i = 1; i < xiao_argc(env); i++) {
        const char *path = xiao_argv(env, i);
        if (xiao_fs_mkdir(path) != 0) {
            xiao_console_print(env, "mkdir: failed: ");
            xiao_console_print(env, path);
            xiao_console_print(env, "\r\n");
        }
    }
    return 0;
}
