#include "xiao.h"
#include "gui_proto.h"

int xiao_app_entry(xiao_env *env) {
    GuiCommand cmd;
    int sw, sh;
    int size = 0;
    int max_size = 200;

    xiao_video_size(env, &sw, &sh);
    // 1. レイヤー(サーフェス)作成 (ID: 0)
    cmd.type = GUI_CMD_CREATE_SURFACE;
    cmd.params.surface.id = 0;
    cmd.params.surface.w = sw;
    cmd.params.surface.h = sh;
    xiao_ipc_send("gui_server", GUI_CMD_CREATE_SURFACE, sizeof(GuiCommand), &cmd);

    // 2. アニメーションループ
    while (size <= max_size) {
        // 背景：赤
        cmd.type = GUI_CMD_DRAW_RECT;
        cmd.params.rect.id = 0;
        cmd.params.rect.x = 0;
        cmd.params.rect.y = 0;
        cmd.params.rect.w = sw;
        cmd.params.rect.h = sh;
        cmd.params.rect.color = 0xFF0000u;
        xiao_ipc_send("gui_server", GUI_CMD_DRAW_RECT, sizeof(GuiCommand), &cmd);

        // 四角：中央、オレンジ
        cmd.type = GUI_CMD_DRAW_RECT;
        cmd.params.rect.id = 0;
        cmd.params.rect.x = (sw / 2) - (size / 2);
        cmd.params.rect.y = (sh / 2) - (size / 2);
        cmd.params.rect.w = size;
        cmd.params.rect.h = size;
        cmd.params.rect.color = 0xFFCB52u;
        xiao_ipc_send("gui_server", GUI_CMD_DRAW_RECT, sizeof(GuiCommand), &cmd);

        // 確定
        cmd.type = GUI_CMD_COMMIT;
        xiao_ipc_send("gui_server", GUI_CMD_COMMIT, sizeof(GuiCommand), &cmd);

        if (size < max_size) {
            size++;
            xiao_wait(env, 25);
        } else {
            // 200pxで停止して待機
            xiao_wait(env, 1000);
            break;
        }
    }

    return 0;
}
