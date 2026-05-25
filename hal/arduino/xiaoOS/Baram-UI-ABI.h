#ifndef BARAM_UI_ABI_H
#define BARAM_UI_ABI_H

#include "xiao.h"

#define BARAM_UI_MAX_NODES 24
#define BARAM_UI_MAX_LAYERS 48
#define BARAM_UI_HTML_MAX 4096
#define BARAM_UI_PATH_MAX 96
#define BARAM_UI_ID_MAX 24
#define BARAM_UI_TEXT_MAX 128
#define BARAM_UI_TITLE_MAX 64
#define BARAM_UI_ACTION_MAX 96

enum {
    BARAM_UI_NODE_NONE = 0,
    BARAM_UI_NODE_TITLE = 1,
    BARAM_UI_NODE_TEXT = 2,
    BARAM_UI_NODE_BUTTON = 3
};

enum {
    BARAM_UI_LAYER_DESKTOP = 1,
    BARAM_UI_LAYER_WINDOW_BG = 2,
    BARAM_UI_LAYER_TITLEBAR = 3,
    BARAM_UI_LAYER_CLOSE = 4,
    BARAM_UI_LAYER_TITLE = 5,
    BARAM_UI_LAYER_TEXT = 6,
    BARAM_UI_LAYER_BUTTON = 7,
    BARAM_UI_LAYER_DOCK = 8,
    BARAM_UI_LAYER_POINTER = 9
};

typedef struct {
    int x;
    int y;
    int w;
    int h;
} baram_ui_rect;

typedef struct {
    int type;
    char id[BARAM_UI_ID_MAX];
    char text[BARAM_UI_TEXT_MAX];
    char action[BARAM_UI_ACTION_MAX];
    baram_ui_rect rect;
} baram_ui_node;

typedef struct {
    int type;
    int node_index;
    int dirty;
    baram_ui_rect rect;
} baram_ui_layer;

typedef struct {
    char html_path[BARAM_UI_PATH_MAX];
    int width;
    int height;
    int has_resize;
    char set_id[BARAM_UI_ID_MAX];
    char set_value[BARAM_UI_TEXT_MAX];
    char startup_action[BARAM_UI_ACTION_MAX];
    int close_requested;
} baram_ui_request;

typedef struct {
    xiao_env *env;
    int running;
    int video_ready;
    int width;
    int height;

    int safety_ticks;

    char current_path[BARAM_UI_PATH_MAX];
    char window_title[BARAM_UI_TITLE_MAX];
    char html_raw[BARAM_UI_HTML_MAX];

    baram_ui_node nodes[BARAM_UI_MAX_NODES];
    int node_count;

    baram_ui_layer layers[BARAM_UI_MAX_LAYERS];
    int layer_count;
    int needs_redraw;
    int window_enabled;

    baram_ui_rect window_rect;
    baram_ui_rect close_rect;
    baram_ui_rect dock_rect;

    int pointer_supported;
    int pointer_valid;
    int pointer_x;
    int pointer_y;
    int pointer_buttons;
    int pointer_prev_x;
    int pointer_prev_y;
    int pointer_prev_buttons;

    int hovered_button;
    int pressed_button;
    int hovered_dock;
    int pressed_dock;
    char pending_action[BARAM_UI_ACTION_MAX];
} baram_ui_context;

void baram_ui_request_init(baram_ui_request *req);
int baram_ui_winapi_parse_request(xiao_env *env, baram_ui_request *req);
int baram_ui_winapi_apply_request(baram_ui_context *ctx, const baram_ui_request *req);
int baram_ui_winapi_execute_action(baram_ui_context *ctx, const char *action);

int baram_ui_win_load_html(baram_ui_context *ctx, const char *path);
int baram_ui_win_set_text(baram_ui_context *ctx, const char *id, const char *text);
void baram_ui_win_build_layers(baram_ui_context *ctx);

int baram_ui_pointer_poll(baram_ui_context *ctx);
void baram_ui_pointer_draw(baram_ui_context *ctx);

void baram_ui_shell_invalidate_all(baram_ui_context *ctx);
int baram_ui_shell_tick(baram_ui_context *ctx);
int baram_ui_shell_consume_action(baram_ui_context *ctx, char *out, xiao_size cap);

int baram_ui_main(xiao_env *env);

#endif
