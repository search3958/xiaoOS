#include "xiao.h"

int xiao_app_entry(xiao_env *env) {
    xiao_size i;
    xiao_console_print(env, "available commands:\r\n");
    for (i = 0; i < xiao_app_count(); i++) {
        const char *name = xiao_app_name(i);
        if (!name) continue;
        xiao_console_print(env, "  ");
        xiao_console_print(env, name);
        xiao_console_print(env, "\r\n");
    }
    return 0;
}
