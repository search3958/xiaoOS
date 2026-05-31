#ifndef XIAO_GUI_PROTO_H
#define XIAO_GUI_PROTO_H

typedef enum {
    GUI_CMD_CREATE_LAYER,
    GUI_CMD_DRAW_RECT,
    GUI_CMD_COMMIT_LAYER,
    GUI_CMD_FRAME_READY
} GuiCmdType;

typedef struct {
    int layer_id;
    int x, y, w, h;
    unsigned int color;
} GuiRectParams;

typedef struct {
    int w, h;
} GuiCreateParams;

typedef struct {
    GuiCmdType type;
    union {
        GuiRectParams rect;
        GuiCreateParams create;
        int layer_id;
    } params;
} GuiCommand;

#endif
