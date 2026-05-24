#include "xiao.h"

int xiao_app_entry(xiao_env *env) {
    int i;
    for (i = 1; i < xiao_argc(env); i++) {
        if (i > 1) xiao_console_print(env, " ");
        xiao_console_print(env, xiao_argv(env, i));
    }
    xiao_console_print(env, "\r\n");
    return 0;
}
