#include "xiao.h"

int xiao_app_entry(xiao_env *env) {
    xiao_size i = 0;
    int show_apps = 0;
    const char *path = ".";
    if (xiao_argc(env) > 1 && xiao_argv(env, 1)[0] == '-' && xiao_argv(env, 1)[1] == 'a') {
        show_apps = 1;
    } else if (xiao_argc(env) > 1) {
        path = xiao_argv(env, 1);
    }

    if (show_apps) {
        for (i = 0; i < xiao_app_count(); i++) {
            xiao_console_print(env, xiao_app_name(i));
            xiao_console_print(env, "\r\n");
        }
        return 0;
    }

    while (1) {
        const char *name = 0;
        int type = 0;
        xiao_size size = 0;
        if (xiao_fs_list(path, i, &name, &type, &size) != 0) break;
        (void)type;
        (void)size;
        xiao_console_print(env, name);
        xiao_console_print(env, "\r\n");
        i++;
    }
    return 0;
}
