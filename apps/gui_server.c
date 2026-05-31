#include "xiao.h"
#include "gui_proto.h"

#define MAX_LAYERS 8
#define MAX_LAYER_W 1280
#define MAX_LAYER_H 720

typedef struct {
    int active;
    int w, h;
    unsigned int *buffer;
} Layer;

static Layer layers[MAX_LAYERS];
static unsigned int frame_buffer[MAX_LAYER_W * MAX_LAYER_H];

int xiao_app_entry(xiao_env *env) {
    xiao_ipc_message msg;
    
    while (1) {
        if (xiao_ipc_receive(&msg) == 0) {
            GuiCommand *cmd = (GuiCommand *)msg.data;
            
            if (cmd->type == GUI_CMD_CREATE_LAYER) {
                for (int i = 0; i < MAX_LAYERS; i++) {
                    if (!layers[i].active) {
                        layers[i].active = 1;
                        layers[i].w = cmd->params.create.w;
                        layers[i].h = cmd->params.create.h;
                        layers[i].buffer = (unsigned int *)0x40000000 + (i * MAX_LAYER_W * MAX_LAYER_H); // 仮のメモリ配置
                        break;
                    }
                }
            } else if (cmd->type == GUI_CMD_DRAW_RECT) {
                int id = cmd->params.rect.layer_id;
                if (id >= 0 && id < MAX_LAYERS && layers[id].active) {
                    Layer *l = &layers[id];
                    for (int iy = cmd->params.rect.y; iy < cmd->params.rect.y + cmd->params.rect.h; iy++) {
                        for (int ix = cmd->params.rect.x; ix < cmd->params.rect.x + cmd->params.rect.w; ix++) {
                            if (ix >= 0 && ix < l->w && iy >= 0 && iy < l->h)
                                l->buffer[iy * l->w + ix] = cmd->params.rect.color;
                        }
                    }
                }
            } else if (cmd->type == GUI_CMD_COMMIT_LAYER) {
                // 合成処理 (簡易的)
                int sw, sh;
                xiao_video_size(env, &sw, &sh);
                for (int i = 0; i < MAX_LAYERS; i++) {
                    if (layers[i].active) {
                        for (int y = 0; y < layers[i].h; y++) {
                            for (int x = 0; x < layers[i].w; x++) {
                                if (x < sw && y < sh)
                                    frame_buffer[y * sw + x] = layers[i].buffer[y * layers[i].w + x];
                            }
                        }
                    }
                }
                xiao_ipc_send("terminal", GUI_CMD_FRAME_READY, sizeof(frame_buffer), frame_buffer);
            }
        }
        xiao_wait(env, 10);
    }
    return 0;
}
