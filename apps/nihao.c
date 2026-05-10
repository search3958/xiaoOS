#include "xiao.h"

int xiao_app_serial_hello(xiao_env *env) {
    xiao_serial_print(env, "NI HAO MA! WO SHI XIAO!\r\n");
    return 0;
}
