#include "xiao.h"

int xiao_app_entry(xiao_env *env) {
    xiao_console_print(env, "reboot: run this command from terminal mode\r\n");
    return 0;
}
