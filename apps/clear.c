#include "xiao.h"

int xiao_app_entry(xiao_env *env) {
    xiao_console_print(env, "\x1b[2J\x1b[H");
    return 0;
}
