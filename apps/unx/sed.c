#include "xiao.h"

static xiao_size strlen_local(const char *s) {
    xiao_size n = 0;
    while (s && s[n]) n++;
    return n;
}

static int starts_with(const char *data, xiao_size remaining, const char *needle, xiao_size needle_len) {
    xiao_size i;
    if (needle_len > remaining) return 0;
    for (i = 0; i < needle_len; i++) {
        if (data[i] != needle[i]) return 0;
    }
    return 1;
}

static int parse_subst(char *expr, const char **old_text, const char **new_text) {
    char delim;
    char *p;
    if (!expr || expr[0] != 's' || !expr[1]) return -1;
    delim = expr[1];
    *old_text = expr + 2;
    p = expr + 2;
    while (*p && *p != delim) p++;
    if (!*p) return -1;
    *p++ = 0;
    *new_text = p;
    while (*p && *p != delim) p++;
    if (*p) *p = 0;
    return 0;
}

static void sed_file(xiao_env *env, const char *old_text, const char *new_text, const char *name) {
    const char *data = 0;
    xiao_size size = 0;
    xiao_size old_len = strlen_local(old_text);
    xiao_size new_len = strlen_local(new_text);
    xiao_size i = 0;
    if (xiao_file_read(name, &data, &size) != 0) {
        xiao_console_print(env, "sed: not found: ");
        xiao_console_print(env, name);
        xiao_console_print(env, "\r\n");
        return;
    }
    if (old_len == 0) {
        xiao_console_write(env, data, size);
        return;
    }
    while (i < size) {
        if (starts_with(data + i, size - i, old_text, old_len)) {
            xiao_console_write(env, new_text, new_len);
            i += old_len;
        } else {
            xiao_console_write(env, data + i, 1);
            i++;
        }
    }
}

int xiao_app_entry(xiao_env *env) {
    char expr[64];
    const char *old_text = 0;
    const char *new_text = 0;
    const char *arg;
    xiao_size i = 0;
    int f;
    if (xiao_argc(env) < 3) {
        xiao_console_print(env, "usage: sed s/OLD/NEW/ FILE...\r\n");
        return 1;
    }
    arg = xiao_argv(env, 1);
    while (arg[i] && i + 1 < sizeof(expr)) {
        expr[i] = arg[i];
        i++;
    }
    expr[i] = 0;
    if (parse_subst(expr, &old_text, &new_text) != 0) {
        xiao_console_print(env, "sed: only s/OLD/NEW/ is supported\r\n");
        return 1;
    }
    for (f = 2; f < xiao_argc(env); f++) {
        sed_file(env, old_text, new_text, xiao_argv(env, f));
        xiao_console_print(env, "\r\n");
    }
    return 0;
}
