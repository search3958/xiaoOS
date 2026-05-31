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
            if (msg.size < sizeof(GuiCommand)) {
                // Ignore malformed messages
                continue;
            }
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
            } else if (cmd->type == GUI_CMD_DESTROY_SURFACE) {
                int id = cmd->params.surface.id;
                for (int i = 0; i < MAX_SURFACES; i++) {
                    if (surfaces[i].active && surfaces[i].id == id) {
                        surfaces[i].active = 0;
                        break;
                    }
                }
            } else if (cmd->type == GUI_CMD_DRAW_RECT) {
                int id = cmd->params.rect.id;
                int x = cmd->params.rect.x;
                int y = cmd->params.rect.y;
                int w = cmd->params.rect.w;
                int h = cmd->params.rect.h;
                if (id >= 0 && id < MAX_SURFACES && surfaces[id].active) {
                    Surface *s = &surfaces[id];
                    // Clamp drawing area to surface dimensions
                    int x_start = (x < 0) ? 0 : x;
                    int y_start = (y < 0) ? 0 : y;
                    int x_end = ((x + w) > s->w) ? s->w : (x + w);
                    int y_end = ((y + h) > s->h) ? s->h : (y + h);
                    
                    for (int iy = y_start; iy < y_end; iy++) {
                        for (int ix = x_start; ix < x_end; ix++) {
                            s->buffer[iy * s->w + ix] = cmd->params.rect.color;
                        }
                    }
                }
            } else if (cmd->type == GUI_CMD_COMMIT) {
                // Atomic commit: Only update the framebuffer once all surfaces are composed.
                // This prevents partial rendering artifacts.
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
                // Notify all clients of the frame availability
                xiao_ipc_send("terminal", GUI_CMD_FRAME_READY, 0, NULL);
                // Also notify other potentially interested GUI clients
                xiao_ipc_send("red", GUI_CMD_FRAME_READY, 0, NULL);
                xiao_ipc_send("red2", GUI_CMD_FRAME_READY, 0, NULL);
            }
        }
        xiao_wait(env, 10);
    }
    return 0;
}
