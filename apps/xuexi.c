#include "xiao.h"

int xiao_app_entry(xiao_env *env) {
    xiao_serial_print(env, "Hello, World.\r\nXIEXI QIANGGUO WANSUI WANWANSUI\r\n");
    return 0;
}
