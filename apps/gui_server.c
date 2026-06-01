#include "xiao.h"
#include "gui_proto.h"
#include <stddef.h>

#ifndef GUI_CMD_RESET_ALL
#define GUI_CMD_RESET_ALL 2
#endif
#ifndef GUI_CMD_HUD_SHOW
#define GUI_CMD_HUD_SHOW 3
#endif

unsigned int gui_framebuffer[MAX_LAYER_W * MAX_LAYER_H];
#define MAX_SURFACES 8

typedef struct {
    int active;
    int id;
    int w, h;
    unsigned int buffer[MAX_LAYER_W * MAX_LAYER_H / 8];
} Surface;

static Surface surfaces[MAX_SURFACES];
static int show_hud = 0;
static int frame_counter = 0;

void hud_render(unsigned int *fb, int *surfaces_active) {
    for (int i = 0; i < MAX_SURFACES; i++) {
        if (surfaces_active[i]) {
            int debug_x = i * 20;
            int debug_y = 0;
            for (int dy = 0; dy < 10; dy++) {
                for (int dx = 0; dx < 10; dx++) {
                    if (debug_x + dx < MAX_LAYER_W && debug_y + dy < MAX_LAYER_H)
                        fb[(debug_y + dy) * MAX_LAYER_W + (debug_x + dx)] = 0xFFFFFFu;
                }
            }
        }
    }
}

int xiao_app_entry(xiao_env *env) {
    while (1) {
        // 1. Process all pending IPC commands (Update surface content)
        xiao_ipc_message msg;
        while (xiao_ipc_receive(&msg) == 0) {
            if (msg.size < sizeof(GuiCommand)) continue;
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
            } else if (cmd->type == GUI_CMD_RESET_ALL) {
                show_hud = 0;
                for (int i = 0; i < MAX_SURFACES; i++) surfaces[i].active = 0;
                gui_clear_framebuffer();
            } else if (cmd->type == GUI_CMD_HUD_SHOW) {
                show_hud = 1;
            } else if (cmd->type == GUI_CMD_DRAW_RECT) {
                int id = cmd->params.rect.id;
                int x = cmd->params.rect.x;
                int y = cmd->params.rect.y;
                int w = cmd->params.rect.w;
                int h = cmd->params.rect.h;
                if (id >= 0 && id < MAX_SURFACES && surfaces[id].active) {
                    Surface *s = &surfaces[id];
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
            }
        }

        // 2. Render loop (Blit framebuffer content to screen at fixed rate)
        unsigned int *fb = gui_get_framebuffer();
        
        // Cycle background to test render loop liveness
        unsigned int bg = (frame_counter++ % 256) << 16;
        for (int i = 0; i < MAX_LAYER_W * MAX_LAYER_H; i++) fb[i] = bg;

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
        
        if (show_hud) {
            int active_arr[MAX_SURFACES];
            for(int i=0; i<MAX_SURFACES; i++) active_arr[i] = surfaces[i].active;
            hud_render(fb, active_arr);
        }

        xiao_video_blit_rgb888(env, 0, 0, MAX_LAYER_W, MAX_LAYER_H, fb, MAX_LAYER_W);
        xiao_wait(env, 16);
    }
    return 0;
}
