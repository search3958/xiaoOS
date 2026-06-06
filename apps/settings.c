#include "xiao.h"

static int streq(const char *a, const char *b) {
    while (*a && *b && *a == *b) { a++; b++; }
    return *a == 0 && *b == 0;
}

int xiao_app_entry(xiao_env *env) {
    if (xiao_argc(env) < 3) {
        xiao_console_print(env, "Usage: settings [0|1] [setting_name]\r\n");
        return -1;
    }

    int value = (int)xiao_argv(env, 1)[0] - '0';
    const char *setting = xiao_argv(env, 2);

    if (streq(setting, "mousekeys")) {
        xiao_settings_set_mousekeys(value);
        xiao_console_print(env, "Mouse keys ");
        xiao_console_print(env, value ? "enabled" : "disabled");
        xiao_console_print(env, "\r\n");
    } else {
        xiao_console_print(env, "Unknown setting\r\n");
    }

    return 0;
}
