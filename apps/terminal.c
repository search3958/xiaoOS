#include "xiao.h"

static xiao_size xiao_strlen_local(const char *s) {
    xiao_size n = 0;
    while (s && s[n]) n++;
    return n;
}

static int xiao_streq_local(const char *a, const char *b) {
    while (*a && *b && *a == *b) {
        a++;
        b++;
    }
    return *a == 0 && *b == 0;
}

int xiao_app_entry(xiao_env *env) {
    char line[64];
    xiao_size n = 0;

    xiao_console_print(env, "xiao terminal ready\r\n");
    xiao_console_print(env, "type app name (example: hello), or 'exit'\r\n");

    while (1) {
        int ch;
        xiao_console_print(env, "> ");
        n = 0;

        while (1) {
            ch = xiao_input_read(env);
            if (ch < 0) {
                xiao_wait(env, 10);
                continue;
            }
            if (ch == '\r' || ch == '\n') {
                xiao_console_print(env, "\r\n");
                break;
            }
            if ((ch == 0x08 || ch == 0x7f) && n > 0) {
                n--;
                xiao_console_print(env, "\\b \\b");
                continue;
            }
            if (ch >= 32 && ch <= 126 && n + 1 < sizeof(line)) {
                line[n++] = (char)ch;
                {
                    char out[2];
                    out[0] = (char)ch;
                    out[1] = 0;
                    xiao_console_print(env, out);
                }
            }
        }

        line[n] = 0;
        if (n == 0) continue;
        if (xiao_streq_local(line, "exit")) break;

        if (xiao_exec_app(line) != 0) {
            xiao_console_print(env, "app not found: ");
            xiao_console_print(env, line);
            xiao_console_print(env, "\r\n");
        }
    }

    xiao_console_print(env, "terminal closed\r\n");
    return (int)xiao_strlen_local(line);
}
