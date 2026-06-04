#include "xiao.h"

static int streq(const char *a, const char *b) {
    while (*a && *b && *a == *b) {
        a++;
        b++;
    }
    return *a == 0 && *b == 0;
}

static int has_app(const char *name) {
    xiao_size i;
    for (i = 0; i < xiao_app_count(); i++) {
        const char *app = xiao_app_name(i);
        if (app && streq(app, name)) return 1;
    }
    return 0;
}

int xiao_app_entry(xiao_env *env) {
    int i;
    int ok_all = 1;

    if (xiao_argc(env) < 2) {
        xiao_console_print(env, "usage: which COMMAND...\r\n");
        return 1;
    }

    for (i = 1; i < xiao_argc(env); i++) {
        const char *name = xiao_argv(env, i);
        if (has_app(name)) {
            xiao_console_print(env, name);
            xiao_console_print(env, "\r\n");
        } else {
            ok_all = 0;
        }
    }
    return ok_all ? 0 : 1;
}
