#ifndef XIAO_GUI_PROTO_H
#define XIAO_GUI_PROTO_H

#define MAX_LAYER_W 1280
#define MAX_LAYER_H 720

typedef enum {
    GUI_CMD_CREATE_SURFACE,
    GUI_CMD_DESTROY_SURFACE,
    GUI_CMD_DRAW_RECT,
    GUI_CMD_COMMIT,
    GUI_CMD_FRAME_READY
} GuiCmdType;

typedef struct {
    int id;
    int w, h;
} GuiSurface;

typedef struct {
    GuiCmdType type;
    union {
        GuiSurface surface;
        struct { int id; int x, y, w, h; unsigned int color; } rect;
    } params;
} GuiCommand;

extern unsigned int gui_framebuffer[MAX_LAYER_W * MAX_LAYER_H];

#endif
