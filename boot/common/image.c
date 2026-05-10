#include "xiao.h"

int xiao_app_serial_hello(xiao_env *env);
int xiao_app_hello(xiao_env *env);

static const char boot_text[] =
    "# xiaoOS boot text\n"
    "exec hello\n"
    "wait 100\n"
    "exec serial_hello\n"
    "wait forever\n";

static const xiao_app app_table[] = {
    { "hello", xiao_app_hello },
    { "serial_hello", xiao_app_serial_hello },
};

const xiao_boot_image xiao_image = {
    boot_text,
    app_table,
    sizeof(app_table) / sizeof(app_table[0]),
};
