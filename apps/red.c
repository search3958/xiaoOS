#include "xiao.h"
#include "gui_proto.h"

int xiao_app_entry(xiao_env *env) {
    static GuiCommand cmd;
    int sw, sh;
    int step;
    int total_steps = 80;

    xiao_video_size(env, &sw, &sh);
    xiao_exec_line("mode gui");

    // Initialize surface
    cmd.type = GUI_CMD_CREATE_SURFACE;
    cmd.params.surface.id = 0;
    cmd.params.surface.w = sw;
    cmd.params.surface.h = sh;
    xiao_ipc_send("gui_server", GUI_CMD_CREATE_SURFACE, sizeof(GuiCommand), &cmd);

    for (step = 1; step <= total_steps; step++) {
        // Draw to surface (deferred)
        cmd.type = GUI_CMD_DRAW_RECT;
        cmd.params.rect.id = 0;
        cmd.params.rect.x = sw / 2 - step / 2;
        cmd.params.rect.y = sh / 2 - step / 2;
        cmd.params.rect.w = step;
        cmd.params.rect.h = step;
        cmd.params.rect.color = 0xFFCB52u;
        xiao_ipc_send("gui_server", GUI_CMD_DRAW_RECT, sizeof(GuiCommand), &cmd);

        // Commit and trigger redraw (engine handles blit)
        cmd.type = GUI_CMD_COMMIT;
        xiao_ipc_send("gui_server", GUI_CMD_COMMIT, sizeof(GuiCommand), &cmd);

        xiao_wait(env, 25);
    }


    xiao_exec_line("mode cli");
    return 0;
}
