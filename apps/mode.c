#include "xiao.h"

static int streq(const char *a, const char *b) {
    while (*a && *b && *a == *b) {
        a++;
        b++;
    }
    return *a == 0 && *b == 0;
}

static const char *mode_name(int mode) {
    if (mode == XIAO_MODE_TEXT) return "text";
    if (mode == XIAO_MODE_CLI) return "cli";
    if (mode == XIAO_MODE_GUI) return "gui";
    return "unknown";
}

static int parse_mode(const char *s) {
    if (!s) return -1;
    if (streq(s, "text")) return XIAO_MODE_TEXT;
    if (streq(s, "cli")) return XIAO_MODE_CLI;
    if (streq(s, "gui")) return XIAO_MODE_GUI;
    return -1;
}

int xiao_app_entry(xiao_env *env) {
    int mode;

    if (xiao_argc(env) == 1) {
        xiao_console_print(env, "mode: ");
        xiao_console_print(env, mode_name(xiao_mode_get()));
        xiao_console_print(env, "\r\n");
        return 0;
    }

    if (xiao_argc(env) != 2) {
        xiao_console_print(env, "usage: mode [text|cli|gui]\r\n");
        return 1;
    }

    mode = parse_mode(xiao_argv(env, 1));
    if (mode < 0 || xiao_mode_set(mode) != 0) {
        xiao_console_print(env, "mode: unknown mode\r\n");
        xiao_console_print(env, "usage: mode [text|cli|gui]\r\n");
        return 1;
    }

    xiao_console_print(env, "mode switched to ");
    xiao_console_print(env, mode_name(mode));
    xiao_console_print(env, "\r\n");

    {
        const char *argv_text[] = { "terminal", "-m", "text" };
        const char *argv_cli[] = { "terminal", "-m", "cli" };
        const char *argv_gui[] = { "terminal", "-m", "gui" };

        if (mode == XIAO_MODE_TEXT) return xiao_exec_app_args("terminal", 3, argv_text);
        if (mode == XIAO_MODE_CLI) return xiao_exec_app_args("terminal", 3, argv_cli);
        return xiao_exec_app_args("terminal", 3, argv_gui);
    }
}
