#include "xiao.h"

int xiao_app_serial_hello(xiao_env *env) {
    xiao_serial_print(env, "hello world from xiaoOS serial app\r\n");
    return 0;
}
