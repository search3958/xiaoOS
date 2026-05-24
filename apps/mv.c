#include "xiao.h"

int xiao_app_entry(xiao_env *env) {
    if (xiao_argc(env) != 3) {
        xiao_console_print(env, "usage: mv SRC DST\r\n");
        return 1;
    }
    if (xiao_fs_rename(xiao_argv(env, 1), xiao_argv(env, 2)) != 0) {
        xiao_console_print(env, "mv: failed\r\n");
        return 1;
    }
    return 0;
}
