#include "Baram-UI-ABI.h"

#ifdef BARAM_UI_EMBEDDED
static int api_streq(const char *a, const char *b) {
    while (*a && *b && *a == *b) {
        a++;
        b++;
    }
    return *a == 0 && *b == 0;
}

static int api_starts_with(const char *s, const char *prefix) {
    while (*prefix) {
        if (*s != *prefix) return 0;
        s++;
        prefix++;
    }
    return 1;
}

static void api_copy_cap(char *dst, xiao_size cap, const char *src) {
    xiao_size i = 0;
    if (!dst || cap == 0) return;
    while (src && src[i] && i + 1 < cap) {
        dst[i] = src[i];
        i++;
    }
    dst[i] = 0;
}

static int api_parse_int(const char *s, int *out) {
    int v = 0;
    int n = 0;
    if (!s || !s[0] || !out) return -1;
    while (s[n] >= '0' && s[n] <= '9') {
        v = v * 10 + (s[n] - '0');
        n++;
    }
    if (n == 0 || s[n] != 0) return -1;
    *out = v;
    return 0;
}

void baram_ui_request_init(baram_ui_request *req) {
    if (!req) return;
    req->html_path[0] = 0;
    req->width = 0;
    req->height = 0;
    req->has_resize = 0;
    req->set_id[0] = 0;
    req->set_value[0] = 0;
    req->startup_action[0] = 0;
    req->close_requested = 0;
}

static void baram_ui_usage(xiao_env *env) {
    xiao_console_print(env, "Baram-UI-WinAPI usage:\r\n");
    xiao_console_print(env, "  Baram-UI-WinAPI [open [HTML_PATH]]\r\n");
    xiao_console_print(env, "  Baram-UI-WinAPI navigate HTML_PATH\r\n");
    xiao_console_print(env, "  Baram-UI-WinAPI resize WIDTH HEIGHT [HTML_PATH]\r\n");
    xiao_console_print(env, "  Baram-UI-WinAPI set ID VALUE [HTML_PATH]\r\n");
    xiao_console_print(env, "  Baram-UI-WinAPI operate ACTION [HTML_PATH]\r\n");
    xiao_console_print(env, "  Baram-UI-WinAPI close\r\n");
}

int baram_ui_winapi_parse_request(xiao_env *env, baram_ui_request *req) {
    int argc;
    const char *cmd;

    if (!env || !req) return -1;
    baram_ui_request_init(req);
    argc = xiao_argc(env);
    if (argc <= 1) {
        api_copy_cap(req->html_path, sizeof(req->html_path), "/gui/desktop.html");
        return 0;
    }

    cmd = xiao_argv(env, 1);
    if (!cmd) return -1;

    if (api_streq(cmd, "open")) {
        if (argc >= 3) api_copy_cap(req->html_path, sizeof(req->html_path), xiao_argv(env, 2));
        else api_copy_cap(req->html_path, sizeof(req->html_path), "/gui/desktop.html");
        return 0;
    }

    if (api_streq(cmd, "navigate")) {
        if (argc != 3) {
            baram_ui_usage(env);
            return -1;
        }
        api_copy_cap(req->html_path, sizeof(req->html_path), xiao_argv(env, 2));
        return 0;
    }

    if (api_streq(cmd, "resize")) {
        if (argc < 4 || argc > 5 ||
            api_parse_int(xiao_argv(env, 2), &req->width) != 0 ||
            api_parse_int(xiao_argv(env, 3), &req->height) != 0) {
            baram_ui_usage(env);
            return -1;
        }
        req->has_resize = 1;
        if (argc == 5) api_copy_cap(req->html_path, sizeof(req->html_path), xiao_argv(env, 4));
        else api_copy_cap(req->html_path, sizeof(req->html_path), "/gui/desktop.html");
        return 0;
    }

    if (api_streq(cmd, "set")) {
        if (argc < 4 || argc > 5) {
            baram_ui_usage(env);
            return -1;
        }
        api_copy_cap(req->set_id, sizeof(req->set_id), xiao_argv(env, 2));
        api_copy_cap(req->set_value, sizeof(req->set_value), xiao_argv(env, 3));
        if (argc == 5) api_copy_cap(req->html_path, sizeof(req->html_path), xiao_argv(env, 4));
        else api_copy_cap(req->html_path, sizeof(req->html_path), "/gui/desktop.html");
        return 0;
    }

    if (api_streq(cmd, "operate")) {
        if (argc < 3 || argc > 4) {
            baram_ui_usage(env);
            return -1;
        }
        api_copy_cap(req->startup_action, sizeof(req->startup_action), xiao_argv(env, 2));
        if (argc == 4) api_copy_cap(req->html_path, sizeof(req->html_path), xiao_argv(env, 3));
        else api_copy_cap(req->html_path, sizeof(req->html_path), "/gui/desktop.html");
        return 0;
    }

    if (api_streq(cmd, "close")) {
        req->close_requested = 1;
        return 0;
    }

    if (argc == 2) {
        api_copy_cap(req->html_path, sizeof(req->html_path), cmd);
        return 0;
    }

    baram_ui_usage(env);
    return -1;
}

int baram_ui_winapi_execute_action(baram_ui_context *ctx, const char *action) {
    if (!ctx || !action || !action[0]) return 0;

    if (api_streq(action, "close")) {
        ctx->running = 0;
        return 0;
    }

    if (api_starts_with(action, "cmd:")) {
        const char *line = action + 4;
        if (!line[0]) return 0;
        return xiao_exec_line(line);
    }

    if (api_starts_with(action, "open:")) {
        const char *path = action + 5;
        if (!path[0]) return -1;
        if (baram_ui_win_load_html(ctx, path) != 0) return -1;
        baram_ui_win_build_layers(ctx);
        baram_ui_shell_invalidate_all(ctx);
        return 0;
    }

    if (api_starts_with(action, "set:")) {
        const char *payload = action + 4;
        const char *eq = payload;
        char id[BARAM_UI_ID_MAX];
        char value[BARAM_UI_TEXT_MAX];
        xiao_size i = 0;
        xiao_size j = 0;

        while (*eq && *eq != '=') eq++;
        if (*eq != '=') return -1;

        while (payload[i] && &payload[i] < eq && i + 1 < sizeof(id)) {
            id[i] = payload[i];
            i++;
        }
        id[i] = 0;

        eq++;
        while (eq[j] && j + 1 < sizeof(value)) {
            value[j] = eq[j];
            j++;
        }
        value[j] = 0;

        if (!id[0]) return -1;
        if (baram_ui_win_set_text(ctx, id, value) != 0) return -1;
        baram_ui_win_build_layers(ctx);
        baram_ui_shell_invalidate_all(ctx);
        return 0;
    }

    return -1;
}

int baram_ui_winapi_apply_request(baram_ui_context *ctx, const baram_ui_request *req) {
    if (!ctx || !req) return -1;

    if (req->close_requested) {
        ctx->running = 0;
        return 0;
    }

    if (req->has_resize && req->width > 0 && req->height > 0) {
        xiao_video_set_mode(ctx->env, req->width, req->height);
        xiao_video_size(ctx->env, &ctx->width, &ctx->height);
    }

    if (baram_ui_win_load_html(ctx, req->html_path) != 0) return -1;
    if (req->set_id[0]) {
        baram_ui_win_set_text(ctx, req->set_id, req->set_value);
    }
    baram_ui_win_build_layers(ctx);
    baram_ui_shell_invalidate_all(ctx);

    if (req->startup_action[0]) {
        return baram_ui_winapi_execute_action(ctx, req->startup_action);
    }

    return 0;
}
#endif

#ifndef BARAM_UI_EMBEDDED
int xiao_app_entry(xiao_env *env) {
    const char *argv[8];
    int argc = xiao_argc(env);
    int i;

    if (argc < 1) argc = 1;
    if (argc > 8) argc = 8;

    argv[0] = "Baram-UI";
    for (i = 1; i < argc; i++) {
        argv[i] = xiao_argv(env, i);
        if (!argv[i]) argv[i] = "";
    }

    return xiao_exec_app_args("Baram-UI", argc, argv);
}
#endif
