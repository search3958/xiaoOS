#include "xiao.h"

#define MAX_PATH 64

static xiao_size str_len(const char *s) {
    xiao_size n = 0;
    while (s && s[n]) n++;
    return n;
}

static int str_eq(const char *a, const char *b) {
    while (*a && *b && *a == *b) { a++; b++; }
    return *a == 0 && *b == 0;
}

static int build_path(char *out, xiao_size cap, const char *base, const char *name) {
    xiao_size i = 0, j = 0;
    if (!out || cap == 0) return -1;
    while (base[i] && i + 1 < cap) { out[i] = base[i]; i++; }
    if (base[i]) return -1;
    if (i > 1 && out[i - 1] != '/') {
        if (i + 1 >= cap) return -1;
        out[i++] = '/';
    }
    while (name[j] && i + 1 < cap) out[i++] = name[j++];
    if (name[j]) return -1;
    out[i] = 0;
    return 0;
}

static void walk(xiao_env *env, const char *path) {
    xiao_size i;
    int type;
    xiao_size size;

    if (xiao_fs_stat(path, &type, &size) != 0) return;
    xiao_console_print(env, path);
    xiao_console_print(env, "\r\n");
    if (type != XIAO_FS_DIR) return;

    for (i = 0;; i++) {
        const char *name;
        int ctype;
        xiao_size csize;
        char child[MAX_PATH];
        if (xiao_fs_list(path, i, &name, &ctype, &csize) != 0) break;
        if (str_eq(name, ".") || str_eq(name, "..")) continue;
        if (build_path(child, sizeof(child), path, name) != 0) continue;
        walk(env, child);
    }
}

int xiao_app_entry(xiao_env *env) {
    const char *path = ".";
    if (xiao_argc(env) > 2) {
        xiao_console_print(env, "usage: find [PATH]\r\n");
        return 1;
    }
    if (xiao_argc(env) == 2) path = xiao_argv(env, 1);
    if (str_len(path) == 0) path = ".";
    walk(env, path);
    return 0;
}
