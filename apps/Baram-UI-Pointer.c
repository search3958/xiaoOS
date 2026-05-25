#include "Baram-UI-ABI.h"

#ifdef BARAM_UI_EMBEDDED
#define BARAM_UI_SERIAL_CMD_MAX 128

static xiao_size pointer_strlen(const char *s) {
    xiao_size n = 0;
    while (s && s[n]) n++;
    return n;
}

static int pointer_streq(const char *a, const char *b) {
    while (*a && *b && *a == *b) {
        a++;
        b++;
    }
    return *a == 0 && *b == 0;
}

static void pointer_copy_cap(char *dst, xiao_size cap, const char *src) {
    xiao_size i = 0;
    if (!dst || cap == 0) return;
    while (src && src[i] && i + 1 < cap) {
        dst[i] = src[i];
        i++;
    }
    dst[i] = 0;
}

static const char *pointer_skip_space(const char *s) {
    while (*s == ' ' || *s == '\t') s++;
    return s;
}

static void pointer_trim_trailing(char *s) {
    xiao_size n = pointer_strlen(s);
    while (n > 0 && (s[n - 1] == ' ' || s[n - 1] == '\t')) {
        s[n - 1] = 0;
        n--;
    }
}

static int pointer_read_word(const char **ps, char *out, xiao_size cap) {
    const char *s;
    xiao_size i = 0;
    if (!ps || !*ps || !out || cap == 0) return 0;
    s = pointer_skip_space(*ps);
    if (!*s) {
        out[0] = 0;
        *ps = s;
        return 0;
    }
    while (*s && *s != ' ' && *s != '\t' && i + 1 < cap) {
        out[i++] = *s++;
    }
    out[i] = 0;
    *ps = s;
    return i > 0;
}

static void pointer_serial(baram_ui_context *ctx, const char *s) {
    if (!ctx || !ctx->env || !s) return;
    xiao_serial_print(ctx->env, s);
}

static void pointer_serial_num(baram_ui_context *ctx, int value) {
    char buf[16];
    int i = 15;
    int neg = 0;
    unsigned int v;
    buf[i--] = 0;
    if (value < 0) {
        neg = 1;
        v = (unsigned int)(-value);
    } else {
        v = (unsigned int)value;
    }
    do {
        buf[i--] = (char)('0' + (v % 10));
        v /= 10;
    } while (v && i >= 0);
    if (neg && i >= 0) buf[i--] = '-';
    pointer_serial(ctx, &buf[i + 1]);
}

static void pointer_print_help(baram_ui_context *ctx) {
    pointer_serial(ctx, "\r\n[Baram-UI serial commands]\r\n");
    pointer_serial(ctx, ":help\r\n");
    pointer_serial(ctx, ":open /gui/home.html\r\n");
    pointer_serial(ctx, ":set ID VALUE\r\n");
    pointer_serial(ctx, ":action open:/gui/second.html\r\n");
    pointer_serial(ctx, ":cmd hello\r\n");
    pointer_serial(ctx, ":pointer\r\n");
    pointer_serial(ctx, ":close\r\n");
    pointer_serial(ctx, "pointer: w/a/s/d move, space click\r\n");
}

static int pointer_exec_serial_command(baram_ui_context *ctx, const char *line) {
    char cmd[20];
    const char *p = line;
    const char *rest;
    if (!ctx || !line) return 0;

    if (!pointer_read_word(&p, cmd, sizeof(cmd))) return 0;
    rest = pointer_skip_space(p);
    if (!rest) rest = "";

    if (pointer_streq(cmd, "help")) {
        pointer_print_help(ctx);
        return 0;
    }

    if (pointer_streq(cmd, "open")) {
        const char *path = rest[0] ? rest : "/gui/desktop.html";
        if (baram_ui_win_load_html(ctx, path) != 0) {
            pointer_serial(ctx, "ui: open failed\r\n");
            return 0;
        }
        baram_ui_win_build_layers(ctx);
        baram_ui_shell_invalidate_all(ctx);
        pointer_serial(ctx, "ui: opened ");
        pointer_serial(ctx, path);
        pointer_serial(ctx, "\r\n");
        return 1;
    }

    if (pointer_streq(cmd, "set")) {
        char id[BARAM_UI_ID_MAX];
        char value[BARAM_UI_TEXT_MAX];
        if (!pointer_read_word(&p, id, sizeof(id))) {
            pointer_serial(ctx, "ui: set needs ID and VALUE\r\n");
            return 0;
        }
        p = pointer_skip_space(p);
        pointer_copy_cap(value, sizeof(value), p);
        pointer_trim_trailing(value);
        if (!value[0]) {
            pointer_serial(ctx, "ui: set needs VALUE\r\n");
            return 0;
        }
        if (baram_ui_win_set_text(ctx, id, value) != 0) {
            pointer_serial(ctx, "ui: unknown ID\r\n");
            return 0;
        }
        baram_ui_win_build_layers(ctx);
        baram_ui_shell_invalidate_all(ctx);
        pointer_serial(ctx, "ui: updated ");
        pointer_serial(ctx, id);
        pointer_serial(ctx, "\r\n");
        return 1;
    }

    if (pointer_streq(cmd, "action")) {
        if (!rest[0]) {
            pointer_serial(ctx, "ui: action needs payload\r\n");
            return 0;
        }
        if (baram_ui_winapi_execute_action(ctx, rest) != 0) {
            pointer_serial(ctx, "ui: action failed\r\n");
            return 0;
        }
        pointer_serial(ctx, "ui: action ok\r\n");
        return 1;
    }

    if (pointer_streq(cmd, "cmd")) {
        if (!rest[0]) {
            pointer_serial(ctx, "ui: cmd needs shell command\r\n");
            return 0;
        }
        if (xiao_exec_line(rest) != 0) {
            pointer_serial(ctx, "ui: cmd failed\r\n");
            return 0;
        }
        pointer_serial(ctx, "ui: cmd ok\r\n");
        return 1;
    }

    if (pointer_streq(cmd, "pointer")) {
        pointer_serial(ctx, "pointer x=");
        pointer_serial_num(ctx, ctx->pointer_x);
        pointer_serial(ctx, " y=");
        pointer_serial_num(ctx, ctx->pointer_y);
        pointer_serial(ctx, " b=");
        pointer_serial_num(ctx, ctx->pointer_buttons);
        pointer_serial(ctx, "\r\n");
        return 0;
    }

    if (pointer_streq(cmd, "close")) {
        ctx->running = 0;
        pointer_serial(ctx, "ui: closing\r\n");
        return 1;
    }

    pointer_serial(ctx, "ui: unknown command, use :help\r\n");
    return 0;
}

static int pointer_clamp(int v, int minv, int maxv) {
    if (v < minv) return minv;
    if (v > maxv) return maxv;
    return v;
}

static int pointer_poll_virtual_input(baram_ui_context *ctx) {
    static int serial_cmd_mode = 0;
    static int serial_cmd_len = 0;
    static char serial_cmd[BARAM_UI_SERIAL_CMD_MAX];
    int ch;
    int nx;
    int ny;
    int nb;
    int changed = 0;

    if (!ctx || !ctx->env || !ctx->env->hal || !ctx->env->hal->input_read) return 0;

    ch = ctx->env->hal->input_read();
    if (ch < 0) return 0;

    if (serial_cmd_mode) {
        if (ch == '\r' || ch == '\n') {
            serial_cmd[serial_cmd_len] = 0;
            pointer_trim_trailing(serial_cmd);
            if (serial_cmd[0]) changed |= pointer_exec_serial_command(ctx, serial_cmd);
            serial_cmd_len = 0;
            serial_cmd_mode = 0;
            pointer_serial(ctx, "ui> ");
            return changed;
        }
        if ((ch == 8 || ch == 127) && serial_cmd_len > 0) {
            serial_cmd_len--;
            return 0;
        }
        if (ch >= 32 && ch <= 126 && serial_cmd_len + 1 < (int)sizeof(serial_cmd)) {
            serial_cmd[serial_cmd_len++] = (char)ch;
        }
        return 0;
    }

    if (ch == ':') {
        serial_cmd_mode = 1;
        serial_cmd_len = 0;
        pointer_serial(ctx, "\r\nui> ");
        return 0;
    }

    nx = ctx->pointer_x;
    ny = ctx->pointer_y;
    nb = ctx->pointer_buttons;

    if (ch == 'a' || ch == 'A') nx -= 6;
    else if (ch == 'd' || ch == 'D') nx += 6;
    else if (ch == 'w' || ch == 'W') ny -= 6;
    else if (ch == 's' || ch == 'S') ny += 6;
    else if (ch == ' ' || ch == '\n' || ch == '\r') nb = (nb & 1) ? 0 : 1;
    else if (ch == 'x' || ch == 'X') nb = 0;

    nx = pointer_clamp(nx, 0, ctx->width > 0 ? ctx->width - 1 : 0);
    ny = pointer_clamp(ny, 0, ctx->height > 0 ? ctx->height - 1 : 0);

    if (nx != ctx->pointer_x || ny != ctx->pointer_y || nb != ctx->pointer_buttons) changed = 1;

    ctx->pointer_prev_x = ctx->pointer_x;
    ctx->pointer_prev_y = ctx->pointer_y;
    ctx->pointer_prev_buttons = ctx->pointer_buttons;
    ctx->pointer_x = nx;
    ctx->pointer_y = ny;
    ctx->pointer_buttons = nb;
    return changed;
}

int baram_ui_pointer_poll(baram_ui_context *ctx) {
    int x;
    int y;
    int buttons;
    int rc;
    int changed = 0;

    if (!ctx || !ctx->env) return 0;

    x = ctx->pointer_x;
    y = ctx->pointer_y;
    buttons = ctx->pointer_buttons;

    rc = xiao_pointer_read(ctx->env, &x, &y, &buttons);
    if (rc < 0) {
        if (!ctx->pointer_valid) {
            ctx->pointer_x = ctx->width > 0 ? ctx->width / 2 : 0;
            ctx->pointer_y = ctx->height > 0 ? ctx->height / 2 : 0;
            ctx->pointer_buttons = 0;
            ctx->pointer_prev_x = ctx->pointer_x;
            ctx->pointer_prev_y = ctx->pointer_y;
            ctx->pointer_prev_buttons = 0;
            ctx->pointer_valid = 1;
            changed = 1;
        }
        ctx->pointer_supported = 0;
        if (pointer_poll_virtual_input(ctx)) changed = 1;
        return changed;
    }

    if (!ctx->pointer_supported) {
        ctx->pointer_supported = 1;
        changed = 1;
    }

    x = pointer_clamp(x, 0, ctx->width > 0 ? ctx->width - 1 : 0);
    y = pointer_clamp(y, 0, ctx->height > 0 ? ctx->height - 1 : 0);

    if (!ctx->pointer_valid) {
        ctx->pointer_valid = 1;
        changed = 1;
    }

    if (x != ctx->pointer_x || y != ctx->pointer_y || buttons != ctx->pointer_buttons) {
        changed = 1;
    }

    ctx->pointer_prev_x = ctx->pointer_x;
    ctx->pointer_prev_y = ctx->pointer_y;
    ctx->pointer_prev_buttons = ctx->pointer_buttons;
    ctx->pointer_x = x;
    ctx->pointer_y = y;
    ctx->pointer_buttons = buttons;

    return changed;
}

void baram_ui_pointer_draw(baram_ui_context *ctx) {
    int x;
    int y;

    if (!ctx || !ctx->env) return;
    if (!ctx->pointer_valid) return;

    x = ctx->pointer_x;
    y = ctx->pointer_y;

    xiao_video_fill_rect_rgb888(ctx->env, x, y, 8, 1, 0x111111u);
    xiao_video_fill_rect_rgb888(ctx->env, x, y + 1, 7, 1, 0x111111u);
    xiao_video_fill_rect_rgb888(ctx->env, x, y + 2, 6, 1, 0x111111u);
    xiao_video_fill_rect_rgb888(ctx->env, x, y + 3, 5, 1, 0x111111u);
    xiao_video_fill_rect_rgb888(ctx->env, x, y + 4, 4, 1, 0x111111u);
    xiao_video_fill_rect_rgb888(ctx->env, x, y + 5, 3, 1, 0x111111u);
    xiao_video_fill_rect_rgb888(ctx->env, x, y + 6, 2, 1, 0x111111u);
    xiao_video_fill_rect_rgb888(ctx->env, x + 1, y + 1, 1, 1, 0xFFFFFFu);
    xiao_video_fill_rect_rgb888(ctx->env, x + 1, y + 2, 1, 1, 0xFFFFFFu);
}
#endif

#ifndef BARAM_UI_EMBEDDED
static xiao_size baram_ui_pointer_strlen(const char *s) {
    xiao_size n = 0;
    while (s && s[n]) n++;
    return n;
}

static void baram_ui_pointer_serial(xiao_env *env, const char *s) {
    if (env && env->hal && env->hal->serial_write) {
        env->hal->serial_write(s, baram_ui_pointer_strlen(s));
    }
}

static void baram_ui_pointer_serial_num(xiao_env *env, int value) {
    char buf[16];
    int i = 15;
    int neg = 0;
    unsigned int v;
    buf[i--] = 0;
    if (value < 0) {
        neg = 1;
        v = (unsigned int)(-value);
    } else {
        v = (unsigned int)value;
    }
    do {
        buf[i--] = (char)('0' + (v % 10));
        v /= 10;
    } while (v && i >= 0);
    if (neg && i >= 0) buf[i--] = '-';
    baram_ui_pointer_serial(env, &buf[i + 1]);
}

int xiao_app_entry(xiao_env *env) {
    int x = 0;
    int y = 0;
    int b = 0;
    int i;

    xiao_mode_set(XIAO_MODE_GUI);
    baram_ui_pointer_serial(env, "Baram-UI-Pointer: monitoring pointer for 3s\r\n");

    for (i = 0; i < 300; i++) {
        int nx = x;
        int ny = y;
        int nb = b;
        if (xiao_pointer_read(env, &nx, &ny, &nb) < 0) {
            baram_ui_pointer_serial(env, "Baram-UI-Pointer: pointer unsupported\r\n");
            return 1;
        }
        if (nx != x || ny != y || nb != b) {
            x = nx;
            y = ny;
            b = nb;
            baram_ui_pointer_serial(env, "pointer x=");
            baram_ui_pointer_serial_num(env, x);
            baram_ui_pointer_serial(env, " y=");
            baram_ui_pointer_serial_num(env, y);
            baram_ui_pointer_serial(env, " b=");
            baram_ui_pointer_serial_num(env, b);
            baram_ui_pointer_serial(env, "\r\n");
        }
        xiao_wait(env, 10);
    }

    baram_ui_pointer_serial(env, "Baram-UI-Pointer: done\r\n");
    return 0;
}
#endif
