#include "xiao.h"

int xiao_app_entry(xiao_env *env) {
    int i;
    if (xiao_argc(env) < 2) {
        xiao_console_print(env, "usage: touch FILE...\r\n");
        return 1;
    }
    for (i = 1; i < xiao_argc(env); i++) {
        const char *path = xiao_argv(env, i);
        int type = 0;
        xiao_size size = 0;
        if (xiao_fs_stat(path, &type, &size) == 0) {
            if (type != XIAO_FS_FILE) {
                xiao_console_print(env, "touch: not a file: ");
                xiao_console_print(env, path);
                xiao_console_print(env, "\r\n");
                return 1;
            }
            continue;
        }
        if (xiao_fs_write(path, "", 0) != 0) {
            xiao_console_print(env, "touch: failed: ");
            xiao_console_print(env, path);
            xiao_console_print(env, "\r\n");
            return 1;
        }
    }
    return 0;
}
