#include "xiao.h"
#include "gui_proto.h"

int xiao_app_entry(xiao_env *env) {
    (void)env;
    // 描画：オレンジの四角
    gui_clear_framebuffer();
    gui_draw_rect(200, 200, 400, 300, 0xFFCB52u);
    return 0;
}
