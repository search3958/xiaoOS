#include "xiao.h"
#include "Baram-UI-ABI.h"

#define BARAM_UI_EMBEDDED 1
#include "../../../apps/Baram-UI-Win.c"
#include "../../../apps/Baram-UI-Pointer.c"
#include "../../../apps/Baram-UI-Shell.c"
#include "../../../apps/Baram-UI-WinAPI.c"
#undef BARAM_UI_EMBEDDED

static void ui_zero(char *dst, xiao_size len) {
    xiao_size i;
    for (i = 0; i < len; i++) dst[i] = 0;
}

static void ui_serial(xiao_env *env, const char *text) {
    if (!env || !text) return;
    xiao_serial_print(env, text);
}

static void ui_init_context(baram_ui_context *ctx, xiao_env *env) {
    ui_zero((char *)ctx, sizeof(*ctx));
    ctx->env = env;
    ctx->running = 1;
    ctx->video_ready = 0;
    ctx->pointer_supported = 0;
    ctx->pointer_valid = 0;
    ctx->window_enabled = 1;
    ctx->hovered_button = -1;
    ctx->pressed_button = -1;
    ctx->hovered_dock = -1;
    ctx->pressed_dock = -1;
    ctx->pending_action[0] = 0;
}

static int ui_prepare_video(baram_ui_context *ctx) {
    if (xiao_video_size(ctx->env, &ctx->width, &ctx->height) != 0) {
        ctx->video_ready = 0;
        return -1;
    }
    if (ctx->width < 1 || ctx->height < 1) {
        ctx->video_ready = 0;
        return -1;
    }
    ctx->video_ready = 1;
    return 0;
}

static void ui_safety_watch(baram_ui_context *ctx) {
    ctx->safety_ticks++;
    if (xiao_mode_get() != XIAO_MODE_GUI) {
        xiao_mode_set(XIAO_MODE_GUI);
    }
    if (ctx->pointer_x < 0) ctx->pointer_x = 0;
    if (ctx->pointer_y < 0) ctx->pointer_y = 0;
    if (ctx->pointer_x >= ctx->width) ctx->pointer_x = ctx->width > 0 ? ctx->width - 1 : 0;
    if (ctx->pointer_y >= ctx->height) ctx->pointer_y = ctx->height > 0 ? ctx->height - 1 : 0;
}

int baram_ui_main(xiao_env *env) {
    static baram_ui_context ui;
    static baram_ui_request req;
    static char action[BARAM_UI_ACTION_MAX];

    if (!env) return 1;

    ui_init_context(&ui, env);
    baram_ui_request_init(&req);

    if (baram_ui_winapi_parse_request(env, &req) != 0) {
        return 1;
    }

    if (xiao_mode_set(XIAO_MODE_GUI) != 0) {
        xiao_console_print(env, "Baram-UI: failed to switch GUI mode\r\n");
        return 1;
    }

    if (ui_prepare_video(&ui) != 0) {
        xiao_serial_print(env, "Baram-UI: video unavailable\r\n");
        return 1;
    }

    if (baram_ui_winapi_apply_request(&ui, &req) != 0) {
        ui_serial(env, "Baram-UI: failed to load view\r\n");
        return 1;
    }

    ui_serial(env, "Baram-UI: gui started\r\n");
    ui_serial(env, "Baram-UI: serial control -> :help + Enter\r\n");
    ui_serial(env, "Baram-UI: pointer keys -> w/a/s/d move, space click\r\n");

    while (ui.running) {
        int changed = 0;

        ui_safety_watch(&ui);

        if (baram_ui_pointer_poll(&ui)) {
            baram_ui_shell_invalidate_all(&ui);
            changed = 1;
        }

        if (baram_ui_shell_tick(&ui)) {
            changed = 1;
        }

        if (baram_ui_shell_consume_action(&ui, action, sizeof(action))) {
            baram_ui_winapi_execute_action(&ui, action);
            changed = 1;
        }

        if (!changed) xiao_wait(env, 12);
    }

    ui_serial(env, "Baram-UI: gui closed\r\n");
    return 0;
}

int xiao_app_entry(xiao_env *env) {
    return baram_ui_main(env);
}
