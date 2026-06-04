#include "xiao.h"

#define MAX_LINES 32
#define MAX_LINE_LEN 64

static int str_cmp(const char *a, const char *b) {
    while (*a && *b && *a == *b) { a++; b++; }
    return (int)(unsigned char)*a - (int)(unsigned char)*b;
}

int xiao_app_entry(xiao_env *env) {
    const char *data;
    xiao_size size;
    char lines[MAX_LINES][MAX_LINE_LEN];
    int count = 0;
    xiao_size i;

    if (xiao_argc(env) != 2) {
        xiao_console_print(env, "usage: sort FILE\r\n");
        return 1;
    }
    if (xiao_fs_read(xiao_argv(env, 1), &data, &size) != 0) {
        xiao_console_print(env, "sort: not found: ");
        xiao_console_print(env, xiao_argv(env, 1));
        xiao_console_print(env, "\r\n");
        return 1;
    }

    i = 0;
    while (i < size && count < MAX_LINES) {
        int p = 0;
        while (i < size && data[i] != '\n') {
            if (data[i] != '\r' && p + 1 < MAX_LINE_LEN) lines[count][p++] = data[i];
            i++;
        }
        lines[count][p] = 0;
        count++;
        if (i < size && data[i] == '\n') i++;
    }

    {
        int a, b;
        for (a = 0; a < count; a++) {
            for (b = a + 1; b < count; b++) {
                if (str_cmp(lines[a], lines[b]) > 0) {
                    char tmp[MAX_LINE_LEN];
                    int k = 0;
                    while (lines[a][k]) { tmp[k] = lines[a][k]; k++; }
                    tmp[k] = 0;
                    k = 0;
                    while (lines[b][k]) { lines[a][k] = lines[b][k]; k++; }
                    lines[a][k] = 0;
                    k = 0;
                    while (tmp[k]) { lines[b][k] = tmp[k]; k++; }
                    lines[b][k] = 0;
                }
            }
        }
    }

    for (i = 0; i < (xiao_size)count; i++) {
        xiao_console_print(env, lines[i]);
        xiao_console_print(env, "\r\n");
    }
    if (size > 0 && count == 0) {
        xiao_console_print(env, "sort: input too large\r\n");
        return 1;
    }
    return 0;
}
