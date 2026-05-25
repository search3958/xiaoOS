#include "xiao.h"

#define GUI_STARTUP_CMD_FILE "/system/gui-mode.cmd"

static int streq(const char *a, const char *b) {
    while (*a && *b && *a == *b) {
        a++;
        b++;
    }
    return *a == 0 && *b == 0;
}

static const char *mode_name(int mode) {
    if (mode == XIAO_MODE_TEXT) return "text";
    if (mode == XIAO_MODE_CLI) return "cli";
    if (mode == XIAO_MODE_GUI) return "gui";
    return "unknown";
}

static int parse_mode(const char *s) {
    if (!s) return -1;
    if (streq(s, "text")) return XIAO_MODE_TEXT;
    if (streq(s, "cli")) return XIAO_MODE_CLI;
    if (streq(s, "gui")) return XIAO_MODE_GUI;
    return -1;
}

static int is_space(char c) {
    return c == ' ' || c == '\t' || c == '\r';
}

static void copy_gui_cmd(char *dst, xiao_size cap, const char *src, xiao_size len) {
    xiao_size i = 0;
    xiao_size start = 0;
    xiao_size end = len;
    if (!dst || cap == 0) return;
    dst[0] = 0;
    while (start < len && is_space(src[start])) start++;
    while (end > start && is_space(src[end - 1])) end--;
    while (start < end && i + 1 < cap) {
        dst[i++] = src[start++];
    }
    dst[i] = 0;
}

static int load_gui_startup_cmd(char *out, xiao_size cap) {
    const char *data = 0;
    xiao_size size = 0;
    xiao_size i = 0;
    if (!out || cap == 0) return -1;
    out[0] = 0;
    if (xiao_fs_read(GUI_STARTUP_CMD_FILE, &data, &size) != 0 || !data || size == 0) return -1;
    while (i < size) {
        xiao_size line_start;
        xiao_size line_end;
        while (i < size && (data[i] == '\n' || data[i] == '\r')) i++;
        if (i >= size) break;
        line_start = i;
        while (i < size && data[i] != '\n' && data[i] != '\r') i++;
        line_end = i;
        while (line_start < line_end && is_space(data[line_start])) line_start++;
        if (line_start < line_end && data[line_start] != '#') {
            copy_gui_cmd(out, cap, data + line_start, line_end - line_start);
            if (out[0]) return 0;
        }
    }
    return -1;
}

int xiao_app_entry(xiao_env *env) {
    int mode;
    int prev_mode;
    char gui_cmd[128];

    if (xiao_argc(env) == 1) {
        xiao_console_print(env, "mode: ");
        xiao_console_print(env, mode_name(xiao_mode_get()));
        xiao_console_print(env, "\r\n");
        return 0;
    }

    if (xiao_argc(env) != 2) {
        xiao_console_print(env, "usage: mode [text|cli|gui]\r\n");
        return 1;
    }

    mode = parse_mode(xiao_argv(env, 1));
    if (mode < 0) {
        xiao_console_print(env, "mode: unknown mode\r\n");
        xiao_console_print(env, "usage: mode [text|cli|gui]\r\n");
        return 1;
    }

    prev_mode = xiao_mode_get();

    if (xiao_mode_set(mode) != 0) {
        xiao_console_print(env, "mode: unknown mode\r\n");
        xiao_console_print(env, "usage: mode [text|cli|gui]\r\n");
        return 1;
    }

    xiao_console_print(env, "mode switched to ");
    xiao_console_print(env, mode_name(mode));
    xiao_console_print(env, "\r\n");

    if (mode == XIAO_MODE_GUI) {
        if (load_gui_startup_cmd(gui_cmd, sizeof(gui_cmd)) != 0 || !gui_cmd[0]) {
            xiao_mode_set(prev_mode);
            xiao_serial_print(env, "mode gui: missing /system/gui-mode.cmd\r\n");
            return 1;
        }
        if (xiao_exec_line(gui_cmd) != 0) {
            xiao_mode_set(prev_mode);
            xiao_serial_print(env, "mode gui: failed to execute startup command\r\n");
            return 1;
        }
    }

    return 0;
}
