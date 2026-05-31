#include "xiao.h"
#include "gui_proto.h"

int xiao_app_entry(xiao_env *env) {
    GuiCommand cmd;
    int sw = 1280; // 仮の画面サイズ
    int sh = 720;
    int step;
    int total_steps = 80;

    // コマンドとしてモード切り替えを実行
    xiao_exec_line("mode gui");

    // 1. レイヤー作成リクエスト (ID: 0)
    cmd.type = GUI_CMD_CREATE_LAYER;
    cmd.params.create.w = sw;
    cmd.params.create.h = sh;
    xiao_ipc_send("gui_server", GUI_CMD_CREATE_LAYER, sizeof(GuiCommand), &cmd);

    // 2. 背景塗りつぶし
    cmd.type = GUI_CMD_DRAW_RECT;
    cmd.params.rect.layer_id = 0;
    cmd.params.rect.x = 0;
    cmd.params.rect.y = 0;
    cmd.params.rect.w = sw;
    cmd.params.rect.h = sh;
    cmd.params.rect.color = 0xAA0000u;
    xiao_ipc_send("gui_server", GUI_CMD_DRAW_RECT, sizeof(GuiCommand), &cmd);

    for (step = 1; step <= total_steps; step++) {
        // 3. 四角形描画
        cmd.type = GUI_CMD_DRAW_RECT;
        cmd.params.rect.layer_id = 0;
        cmd.params.rect.x = sw / 2 - step / 2;
        cmd.params.rect.y = sh / 2 - step / 2;
        cmd.params.rect.w = step;
        cmd.params.rect.h = step;
        cmd.params.rect.color = 0xFFCB52u;
        xiao_ipc_send("gui_server", GUI_CMD_DRAW_RECT, sizeof(GuiCommand), &cmd);

        // 4. 確定して描画更新
        cmd.type = GUI_CMD_COMMIT_LAYER;
        xiao_ipc_send("gui_server", GUI_CMD_COMMIT_LAYER, sizeof(GuiCommand), &cmd);

        xiao_wait(env, 25);
    }

    // コマンドとしてCLIモードに戻す
    xiao_exec_line("mode cli");

    return 0;
}
