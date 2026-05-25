#include "xiao.h"

int xiao_app_entry(xiao_env *env) {
    const char *argv[3];
    (void)env;
    argv[0] = "Baram-UI-WinAPI";
    argv[1] = "open";
    argv[2] = "/gui/second.html";
    return xiao_exec_app_args("Baram-UI-WinAPI", 3, argv);
}
