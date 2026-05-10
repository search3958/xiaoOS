#include "xiao.h"

int xiao_app_entry(xiao_env *env) {
    xiao_size i;
    int show_apps = 0;
    if (xiao_argc(env) > 1 && xiao_argv(env, 1)[0] == '-' && xiao_argv(env, 1)[1] == 'a') {
        show_apps = 1;
    }

    if (show_apps) {
        for (i = 0; i < xiao_app_count(); i++) {
            xiao_console_print(env, xiao_app_name(i));
            xiao_console_print(env, "\r\n");
        }
        return 0;
    }

    for (i = 0; i < xiao_file_count(); i++) {
        xiao_console_print(env, xiao_file_name(i));
        xiao_console_print(env, "\r\n");
    }
    return 0;
}
