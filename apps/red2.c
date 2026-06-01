#include "xiao.h"
#include "gui_proto.h"

int xiao_app_entry(xiao_env *env) {
    static GuiCommand cmd; 
    int sw, sh;
    int size = 0;
    int max_size = 200;

    xiao_video_size(env, &sw, &sh);
    xiao_exec_line("mode gui");

    // 1. レイヤー(サーフェス)作成 (ID: 0)
    cmd.type = GUI_CMD_CREATE_SURFACE;
    cmd.params.surface.id = 0;
    cmd.params.surface.w = sw;
    cmd.params.surface.h = sh; 
    xiao_ipc_send("gui_server", GUI_CMD_CREATE_SURFACE, sizeof(GuiCommand), &cmd);

    // 2. アニメーションループ
    while (size <= max_size) {
        // 背景：赤 (Deferred)
        cmd.type = GUI_CMD_DRAW_RECT;
        cmd.params.rect.id = 0;
        cmd.params.rect.x = 0;
        cmd.params.rect.y = 0;
        cmd.params.rect.w = sw;
        cmd.params.rect.h = sh;
        cmd.params.rect.color = 0xFF0000u;
        xiao_ipc_send("gui_server", GUI_CMD_DRAW_RECT, sizeof(GuiCommand), &cmd);

        // 四角：中央、オレンジ (Deferred)
        cmd.type = GUI_CMD_DRAW_RECT;
        cmd.params.rect.id = 0;
        cmd.params.rect.x = (sw / 2) - (size / 2);
        cmd.params.rect.y = (sh / 2) - (size / 2);
        cmd.params.rect.w = size;
        cmd.params.rect.h = size;
        cmd.params.rect.color = 0xFFCB52u;
        xiao_ipc_send("gui_server", GUI_CMD_DRAW_RECT, sizeof(GuiCommand), &cmd);

        // 確定 (Trigger Redraw)
        cmd.type = GUI_CMD_COMMIT;
        xiao_ipc_send("gui_server", GUI_CMD_COMMIT, sizeof(GuiCommand), &cmd);
        
        xiao_wait(env, 25);
        size++;
    }
    
    xiao_exec_line("mode cli");
    return 0;
}
