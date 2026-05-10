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

static void print_row(xiao_env *env, const char *name, xiao_size total, xiao_size used) {
    xiao_console_print(env, name);
    xiao_console_print(env, " total=");
    print_uint(env, total);
    xiao_console_print(env, " used=");
    print_uint(env, used);
    xiao_console_print(env, " free=");
    print_uint(env, total > used ? total - used : 0);
    xiao_console_print(env, "\r\n");
}

int xiao_app_entry(xiao_env *env) {
    xiao_fs_info info;
    xiao_fs_info_read(&info);
    print_row(env, "fs-ram", info.ram_total, info.ram_used);
    print_row(env, "fs-nodes", info.nodes_total, info.nodes_used);
    print_row(env, "tasks", xiao_task_slot_count(), xiao_task_active_count());
    xiao_console_print(env, "apps total=");
    print_uint(env, xiao_app_count());
    xiao_console_print(env, "\r\nrom-file-bytes used=");
    print_uint(env, info.rom_used);
    xiao_console_print(env, "\r\n");
    return 0;
}
