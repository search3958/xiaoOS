#include "xiao.h"

int xiao_app_entry(xiao_env *env) {
    const char *path = "/";
    if (xiao_argc(env) > 1) path = xiao_argv(env, 1);
    if (xiao_fs_chdir(path) != 0) {
        xiao_console_print(env, "cd: not a directory: ");
        xiao_console_print(env, path);
        xiao_console_print(env, "\r\n");
        return 1;
    }
    return 0;
}
