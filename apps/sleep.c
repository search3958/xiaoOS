#include "xiao.h"

static unsigned long parse_uint(const char *s, int *ok) {
    unsigned long v = 0;
    int seen = 0;
    while (s && *s) {
        if (*s < '0' || *s > '9') {
            *ok = 0;
            return 0;
        }
        seen = 1;
        v = v * 10 + (unsigned long)(*s - '0');
        s++;
    }
    *ok = seen;
    return v;
}

int xiao_app_entry(xiao_env *env) {
    unsigned long sec;
    int ok = 0;

    if (xiao_argc(env) != 2) {
        xiao_console_print(env, "usage: sleep SECONDS\r\n");
        return 1;
    }

    sec = parse_uint(xiao_argv(env, 1), &ok);
    if (!ok) {
        xiao_console_print(env, "sleep: invalid number\r\n");
        return 1;
    }

    xiao_wait(env, (xiao_tick)(sec * 1000ul));
    return 0;
}
