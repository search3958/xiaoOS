#include "xiao.h"
#include "gui_proto.h"
#include <stddef.h>

unsigned int gui_framebuffer[MAX_LAYER_W * MAX_LAYER_H];

#define MAX_SURFACES 8

typedef struct {
    int active;
    int id;
    int w, h;
    unsigned int buffer[MAX_LAYER_W * MAX_LAYER_H / 8];
} Surface;

static Surface surfaces[MAX_SURFACES];

int xiao_app_entry(xiao_env *env) {
    xiao_ipc_message msg;
    
    while (1) {
        if (xiao_ipc_receive(&msg) == 0) {
            GuiCommand *cmd = (GuiCommand *)msg.data;
            
            if (cmd->type == GUI_CMD_CREATE_SURFACE) {
                for (int i = 0; i < MAX_SURFACES; i++) {
                    if (!surfaces[i].active) {
                        surfaces[i].active = 1;
                        surfaces[i].id = cmd->params.surface.id;
                        surfaces[i].w = cmd->params.surface.w;
                        surfaces[i].h = cmd->params.surface.h;
                        break;
                    }
                }
            } else if (cmd->type == GUI_CMD_DRAW_RECT) {
                int id = cmd->params.rect.id;
                if (id >= 0 && id < MAX_SURFACES && surfaces[id].active) {
                    Surface *s = &surfaces[id];
                    for (int iy = cmd->params.rect.y; iy < cmd->params.rect.y + cmd->params.rect.h; iy++) {
                        for (int ix = cmd->params.rect.x; ix < cmd->params.rect.x + cmd->params.rect.w; ix++) {
                            if (ix >= 0 && ix < s->w && iy >= 0 && iy < s->h)
                                s->buffer[iy * s->w + ix] = cmd->params.rect.color;
                        }
                    }
                }
            } else if (cmd->type == GUI_CMD_COMMIT) {
                unsigned int *fb = gui_get_framebuffer();
                gui_clear_framebuffer();

                for (int i = 0; i < MAX_SURFACES; i++) {
                    if (surfaces[i].active) {
                        for (int y = 0; y < surfaces[i].h; y++) {
                            for (int x = 0; x < surfaces[i].w; x++) {
                                if (x < MAX_LAYER_W && y < MAX_LAYER_H)
                                    fb[y * MAX_LAYER_W + x] = surfaces[i].buffer[y * surfaces[i].w + x];
                            }
                        }
                    }
                }
                xiao_ipc_send("terminal", GUI_CMD_FRAME_READY, 0, NULL);
            }
        }
        xiao_wait(env, 10);
    }
    return 0;
}
