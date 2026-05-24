#include "xiao.h"

static void print_uint(xiao_env *env, xiao_size v) {
    char buf[24];
    int i = 23;
    buf[i--] = 0;
    if (v == 0) {
        xiao_console_print(env, "0");
        return;
    }
    while (v > 0 && i >= 0) {
        buf[i--] = (char)('0' + (v % 10));
        v /= 10;
    }
    xiao_console_print(env, &buf[i + 1]);
}

int xiao_app_entry(xiao_env *env) {
    xiao_size i;
    xiao_console_print(env, "pid state   app\r\n");
    xiao_console_print(env, "0   run     ");
    xiao_console_print(env, xiao_current_app());
    xiao_console_print(env, "\r\n");

    for (i = 0; i < xiao_task_slot_count(); i++) {
        const char *name = xiao_task_name(i);
        if (!name || !name[0]) continue;
        print_uint(env, i + 1);
        xiao_console_print(env, "   ready   ");
        xiao_console_print(env, name);
        xiao_console_print(env, "\r\n");
    }
    return 0;
}
