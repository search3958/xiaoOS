#include "xiao.h"

static void print_uint(xiao_env *env, xiao_size value) {
    char buf[24];
    xiao_size i = sizeof(buf);
    buf[--i] = 0;
    if (value == 0) {
        xiao_console_print(env, "0");
        return;
    }
    while (value && i > 0) {
        buf[--i] = (char)('0' + (value % 10));
        value /= 10;
    }
    xiao_console_print(env, &buf[i]);
}

int xiao_app_entry(xiao_env *env) {
    xiao_size i;
    xiao_console_print(env, "pid state   app\r\n");
    xiao_console_print(env, "0   run     ");
    xiao_console_print(env, xiao_current_app());
    if (env && env->ipc.from) {
        xiao_console_print(env, " from ");
        xiao_console_print(env, env->ipc.from);
    }
    xiao_console_print(env, "\r\n");

    for (i = 0; i < xiao_task_slot_count(); i++) {
        const char *name = xiao_task_name(i);
        if (!name) continue;
        print_uint(env, i + 1);
        xiao_console_print(env, "   ready   ");
        xiao_console_print(env, name);
        xiao_console_print(env, "\r\n");
    }

    xiao_console_print(env, "tasks active/slots ");
    print_uint(env, xiao_task_active_count());
    xiao_console_print(env, "/");
    print_uint(env, xiao_task_slot_count());
    xiao_console_print(env, "\r\napps ");
    print_uint(env, xiao_app_count());
    xiao_console_print(env, "\r\n");
    return 0;
}
