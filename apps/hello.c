#include "xiao.h"

int xiao_app_hello(xiao_env *env) {
    xiao_console_print(env, "hello world from xiaoOS app\r\n");
    return 0;
}
