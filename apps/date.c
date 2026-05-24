#include "xiao.h"

int xiao_app_entry(xiao_env *env) {
    (void)env;
    xiao_console_print(env, "date: not supported in this build (no RTC/time API)\r\n");
    return 1;
}
