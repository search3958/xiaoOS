#include "xiao.h"

int xiao_app_entry(xiao_env *env) {
    xiao_console_print(env, xiao_fs_cwd());
    xiao_console_print(env, "\r\n");
    return 0;
}
