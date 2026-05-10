#include "xiao.h"

__attribute__((weak)) int xiao_app_hello(xiao_env *env) {
    (void)env;
    return 0;
}

__attribute__((weak)) int xiao_app_serial_hello(xiao_env *env) {
    (void)env;
    return 0;
}
