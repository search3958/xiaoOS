#include "Baram-UI-ABI.h"

#ifdef BARAM_UI_EMBEDDED
static xiao_size win_strlen(const char *s) {
    xiao_size n = 0;
    while (s && s[n]) n++;
    return n;
}

static int win_streq(const char *a, const char *b) {
    while (*a && *b && *a == *b) {
        a++;
        b++;
    }
    return *a == 0 && *b == 0;
}

static void win_copy_cap(char *dst, xiao_size cap, const char *src) {
    xiao_size i = 0;
    if (!dst || cap == 0) return;
    while (src && src[i] && i + 1 < cap) {
        dst[i] = src[i];
        i++;
    }
    dst[i] = 0;
}

static void win_zero(char *dst, xiao_size len) {
    xiao_size i;
    for (i = 0; i < len; i++) dst[i] = 0;
}

static int win_is_space(char c) {
    return c == ' ' || c == '\t' || c == '\r' || c == '\n';
}

static const char *win_find_text(const char *haystack, const char *needle) {
    xiao_size nlen = win_strlen(needle);
    if (!haystack || !needle || nlen == 0) return 0;
    while (*haystack) {
        xiao_size i = 0;
        while (i < nlen && haystack[i] == needle[i]) i++;
        if (i == nlen) return haystack;
        haystack++;
    }
    return 0;
}

static const char *win_find_char(const char *s, char ch) {
    while (s && *s) {
        if (*s == ch) return s;
        s++;
    }
    return 0;
}

static void win_compact_text(const char *start, const char *end, char *out, xiao_size cap) {
    xiao_size n = 0;
    int prev_space = 0;
    if (!out || cap == 0) return;
    out[0] = 0;
    if (!start || !end || start >= end) return;
    while (start < end && n + 1 < cap) {
        char c = *start++;
        if (win_is_space(c)) {
            if (!prev_space && n > 0 && n + 1 < cap) {
                out[n++] = ' ';
            }
            prev_space = 1;
            continue;
        }
        out[n++] = c;
        prev_space = 0;
    }
    while (n > 0 && out[n - 1] == ' ') n--;
    out[n] = 0;
}

static void win_read_attr(const char *start, const char *end, const char *name, char *out, xiao_size cap) {
    xiao_size nlen = win_strlen(name);
    const char *p = start;
    if (!out || cap == 0) return;
    out[0] = 0;
    if (!start || !end || !name || nlen == 0 || start >= end) return;

    while (p && p < end) {
        const char *eq;
        const char *vstart;
        const char *vend;

        while (p < end && win_is_space(*p)) p++;
        if (p >= end || *p == '>' || *p == '/') break;

        eq = p;
        while (eq < end && *eq != '=' && !win_is_space(*eq) && *eq != '>') eq++;
        if (eq >= end || *eq != '=') {
            while (eq < end && *eq != ' ' && *eq != '\t' && *eq != '>') eq++;
            p = eq;
            continue;
        }

        if ((xiao_size)(eq - p) == nlen) {
            xiao_size i;
            int same = 1;
            for (i = 0; i < nlen; i++) {
                if (p[i] != name[i]) {
                    same = 0;
                    break;
                }
            }
            if (same) {
                vstart = eq + 1;
                while (vstart < end && win_is_space(*vstart)) vstart++;
                if (vstart >= end) return;
                if (*vstart == '"' || *vstart == '\'') {
                    char quote = *vstart++;
                    vend = vstart;
                    while (vend < end && *vend != quote) vend++;
                    win_compact_text(vstart, vend, out, cap);
                    return;
                }
                vend = vstart;
                while (vend < end && !win_is_space(*vend) && *vend != '>') vend++;
                win_compact_text(vstart, vend, out, cap);
                return;
            }
        }

        p = eq + 1;
        while (p < end && *p != ' ' && *p != '\t' && *p != '>') p++;
    }
}

static int win_auto_id(char *out, xiao_size cap, const char *prefix, int index) {
    char num[12];
    int n = 0;
    int i;
    if (!out || cap == 0) return -1;
    win_copy_cap(out, cap, prefix);
    while (out[n]) n++;
    if (n + 2 >= (int)cap) return -1;
    out[n++] = '_';
    if (index <= 0) {
        out[n++] = '0';
        out[n] = 0;
        return 0;
    }
    i = 0;
    while (index > 0 && i + 1 < (int)sizeof(num)) {
        num[i++] = (char)('0' + (index % 10));
        index /= 10;
    }
    while (i > 0 && n + 1 < (int)cap) out[n++] = num[--i];
    out[n] = 0;
    return 0;
}

static void win_reset_doc(baram_ui_context *ctx) {
    int i;
    ctx->node_count = 0;
    ctx->layer_count = 0;
    ctx->needs_redraw = 1;
    ctx->window_enabled = 1;
    ctx->window_title[0] = 0;
    for (i = 0; i < BARAM_UI_MAX_NODES; i++) {
        ctx->nodes[i].type = BARAM_UI_NODE_NONE;
        ctx->nodes[i].id[0] = 0;
        ctx->nodes[i].text[0] = 0;
        ctx->nodes[i].action[0] = 0;
        ctx->nodes[i].rect.x = 0;
        ctx->nodes[i].rect.y = 0;
        ctx->nodes[i].rect.w = 0;
        ctx->nodes[i].rect.h = 0;
    }
}

static baram_ui_node *win_new_node(baram_ui_context *ctx, int type) {
    baram_ui_node *node;
    if (!ctx || ctx->node_count >= BARAM_UI_MAX_NODES) return 0;
    node = &ctx->nodes[ctx->node_count++];
    node->type = type;
    node->id[0] = 0;
    node->text[0] = 0;
    node->action[0] = 0;
    node->rect.x = 0;
    node->rect.y = 0;
    node->rect.w = 0;
    node->rect.h = 0;
    return node;
}

static void win_parse_nodes_for_tag(baram_ui_context *ctx, const char *html, const char *tag, const char *close_tag, int node_type, int with_action) {
    const char *p = html;
    int created = 0;
    while (p && *p) {
        const char *open = win_find_text(p, tag);
        const char *open_end;
        const char *content_start;
        const char *close;
        baram_ui_node *node;

        if (!open) break;
        open_end = win_find_char(open, '>');
        if (!open_end) break;

        content_start = open_end + 1;
        close = win_find_text(content_start, close_tag);
        if (!close) break;

        node = win_new_node(ctx, node_type);
        if (!node) return;

        win_read_attr(open, open_end, "id", node->id, sizeof(node->id));
        win_compact_text(content_start, close, node->text, sizeof(node->text));
        if (with_action) {
            win_read_attr(open, open_end, "action", node->action, sizeof(node->action));
        }

        if (!node->id[0]) {
            const char *prefix = node_type == BARAM_UI_NODE_BUTTON ? "button" : (node_type == BARAM_UI_NODE_TITLE ? "title" : "text");
            win_auto_id(node->id, sizeof(node->id), prefix, created);
        }
        if (!node->text[0]) win_copy_cap(node->text, sizeof(node->text), "-");

        created++;
        p = close + win_strlen(close_tag);
    }
}

static void win_parse_window_title(baram_ui_context *ctx, const char *html) {
    const char *open = win_find_text(html, "<window");
    const char *open_end;
    if (!open) return;
    open_end = win_find_char(open, '>');
    if (!open_end) return;
    win_read_attr(open, open_end, "title", ctx->window_title, sizeof(ctx->window_title));
}

static void win_ensure_defaults(baram_ui_context *ctx) {
    baram_ui_node *node;
    int i;
    int have_title = 0;
    int have_text = 0;
    int have_button = 0;

    for (i = 0; i < ctx->node_count; i++) {
        if (ctx->nodes[i].type == BARAM_UI_NODE_TITLE) have_title = 1;
        else if (ctx->nodes[i].type == BARAM_UI_NODE_TEXT) have_text = 1;
        else if (ctx->nodes[i].type == BARAM_UI_NODE_BUTTON) have_button = 1;
    }

    if (!have_title) {
        node = win_new_node(ctx, BARAM_UI_NODE_TITLE);
        if (node) {
            win_copy_cap(node->id, sizeof(node->id), "main_title");
            win_copy_cap(node->text, sizeof(node->text), "Baram UI");
        }
    }

    if (!have_text) {
        node = win_new_node(ctx, BARAM_UI_NODE_TEXT);
        if (node) {
            win_copy_cap(node->id, sizeof(node->id), "main_text");
            win_copy_cap(node->text, sizeof(node->text), "GUI mode: keyboard and text console are disabled.");
        }
    }

    if (!have_button) {
        node = win_new_node(ctx, BARAM_UI_NODE_BUTTON);
        if (node) {
            win_copy_cap(node->id, sizeof(node->id), "run_hello");
            win_copy_cap(node->text, sizeof(node->text), "RUN HELLO");
            win_copy_cap(node->action, sizeof(node->action), "cmd:hello");
        }
    }

    if (!ctx->window_title[0]) {
        for (i = 0; i < ctx->node_count; i++) {
            if (ctx->nodes[i].type == BARAM_UI_NODE_TITLE) {
                win_copy_cap(ctx->window_title, sizeof(ctx->window_title), ctx->nodes[i].text);
                break;
            }
        }
    }

    if (!ctx->window_title[0]) {
        win_copy_cap(ctx->window_title, sizeof(ctx->window_title), "Baram GUI");
    }
}

static int win_text_width_px(const char *text, int scale) {
    xiao_size n = win_strlen(text);
    if (scale < 1) scale = 1;
    return (int)n * (6 * scale);
}

static void win_add_layer(baram_ui_context *ctx, int type, int node_index, int x, int y, int w, int h) {
    baram_ui_layer *layer;
    if (!ctx || ctx->layer_count >= BARAM_UI_MAX_LAYERS) return;
    layer = &ctx->layers[ctx->layer_count++];
    layer->type = type;
    layer->node_index = node_index;
    layer->dirty = 1;
    layer->rect.x = x;
    layer->rect.y = y;
    layer->rect.w = w;
    layer->rect.h = h;
}

int baram_ui_win_load_html(baram_ui_context *ctx, const char *path) {
    static const char fallback[] =
        "<window title=\"Baram GUI\">"
        "<title id=\"welcome\">Baram GUI Ready</title>"
        "<text id=\"desc\">Open HTML from C apps via Baram-UI-WinAPI.</text>"
        "<button id=\"hello_btn\" action=\"cmd:hello\">RUN HELLO</button>"
        "<button id=\"next_btn\" action=\"open:/gui/second.html\">OPEN NEXT PAGE</button>"
        "</window>";
    const char *src = 0;
    xiao_size size = 0;
    xiao_size i;
    xiao_size copy_n;

    if (!ctx) return -1;

    if (!path || !path[0]) path = "/gui/desktop.html";

    if (xiao_fs_read(path, &src, &size) != 0 || !src || size == 0) {
        src = fallback;
        size = win_strlen(fallback);
        win_copy_cap(ctx->current_path, sizeof(ctx->current_path), "(fallback)");
    } else {
        win_copy_cap(ctx->current_path, sizeof(ctx->current_path), path);
    }

    copy_n = size;
    if (copy_n >= BARAM_UI_HTML_MAX) copy_n = BARAM_UI_HTML_MAX - 1;
    for (i = 0; i < copy_n; i++) ctx->html_raw[i] = src[i];
    ctx->html_raw[copy_n] = 0;

    win_reset_doc(ctx);
    if (!win_find_text(ctx->html_raw, "<window")) {
        ctx->window_enabled = 0;
        win_copy_cap(ctx->window_title, sizeof(ctx->window_title), "Baram Desktop");
        return 0;
    }
    ctx->window_enabled = 1;
    win_parse_window_title(ctx, ctx->html_raw);
    win_parse_nodes_for_tag(ctx, ctx->html_raw, "<title", "</title>", BARAM_UI_NODE_TITLE, 0);
    win_parse_nodes_for_tag(ctx, ctx->html_raw, "<h1", "</h1>", BARAM_UI_NODE_TITLE, 0);
    win_parse_nodes_for_tag(ctx, ctx->html_raw, "<text", "</text>", BARAM_UI_NODE_TEXT, 0);
    win_parse_nodes_for_tag(ctx, ctx->html_raw, "<p", "</p>", BARAM_UI_NODE_TEXT, 0);
    win_parse_nodes_for_tag(ctx, ctx->html_raw, "<button", "</button>", BARAM_UI_NODE_BUTTON, 1);
    win_ensure_defaults(ctx);
    return 0;
}

int baram_ui_win_set_text(baram_ui_context *ctx, const char *id, const char *text) {
    int i;
    if (!ctx || !id || !id[0] || !text) return -1;
    for (i = 0; i < ctx->node_count; i++) {
        if (win_streq(ctx->nodes[i].id, id)) {
            win_copy_cap(ctx->nodes[i].text, sizeof(ctx->nodes[i].text), text);
            return 0;
        }
    }
    return -1;
}

void baram_ui_win_build_layers(baram_ui_context *ctx) {
    int margin;
    int titlebar_h;
    int cursor_y;
    int body_left;
    int body_right;
    int i;

    if (!ctx) return;

    margin = ctx->width > 320 ? 18 : 8;
    titlebar_h = 24;

    ctx->layer_count = 0;
    win_zero((char *)ctx->layers, sizeof(ctx->layers));
    win_add_layer(ctx, BARAM_UI_LAYER_DESKTOP, -1, 0, 0, ctx->width, ctx->height);

    if (!ctx->window_enabled) {
        ctx->window_rect.x = 0;
        ctx->window_rect.y = 0;
        ctx->window_rect.w = 0;
        ctx->window_rect.h = 0;
        ctx->close_rect.x = 0;
        ctx->close_rect.y = 0;
        ctx->close_rect.w = 0;
        ctx->close_rect.h = 0;
        ctx->dock_rect.h = 28;
        ctx->dock_rect.w = ctx->width - margin * 2;
        if (ctx->dock_rect.w < 64) ctx->dock_rect.w = 64;
        ctx->dock_rect.x = margin;
        if (ctx->dock_rect.x < 0) ctx->dock_rect.x = 0;
        ctx->dock_rect.y = ctx->height - margin - ctx->dock_rect.h;
        if (ctx->dock_rect.y < 0) ctx->dock_rect.y = 0;
        win_add_layer(ctx, BARAM_UI_LAYER_DOCK, -1, ctx->dock_rect.x, ctx->dock_rect.y, ctx->dock_rect.w, ctx->dock_rect.h);
        win_add_layer(ctx, BARAM_UI_LAYER_POINTER, -1, 0, 0, ctx->width, ctx->height);
        ctx->needs_redraw = 1;
        return;
    }

    ctx->window_rect.x = margin;
    ctx->window_rect.y = margin;
    ctx->window_rect.w = ctx->width - margin * 2;
    ctx->window_rect.h = ctx->height - margin * 2;
    if (ctx->window_rect.w < 120) ctx->window_rect.w = 120;
    if (ctx->window_rect.h < 90) ctx->window_rect.h = 90;

    ctx->close_rect.w = 16;
    ctx->close_rect.h = 16;
    ctx->close_rect.x = ctx->window_rect.x + ctx->window_rect.w - ctx->close_rect.w - 4;
    ctx->close_rect.y = ctx->window_rect.y + 4;
    ctx->dock_rect.x = ctx->window_rect.x + 6;
    ctx->dock_rect.h = 28;
    ctx->dock_rect.w = ctx->window_rect.w - 12;
    ctx->dock_rect.y = ctx->window_rect.y + ctx->window_rect.h - ctx->dock_rect.h - 6;
    if (ctx->dock_rect.w < 64) ctx->dock_rect.w = 64;
    if (ctx->dock_rect.y < ctx->window_rect.y + 26) ctx->dock_rect.y = ctx->window_rect.y + 26;

    body_left = ctx->window_rect.x + 10;
    body_right = ctx->window_rect.x + ctx->window_rect.w - 10;
    cursor_y = ctx->window_rect.y + titlebar_h + 10;

    for (i = 0; i < ctx->node_count; i++) {
        baram_ui_node *node = &ctx->nodes[i];
        int h;
        if (node->type == BARAM_UI_NODE_TITLE) {
            h = 14;
            node->rect.x = body_left;
            node->rect.y = cursor_y;
            node->rect.w = body_right - body_left;
            node->rect.h = h;
            cursor_y += h + 8;
        } else if (node->type == BARAM_UI_NODE_TEXT) {
            h = 12;
            node->rect.x = body_left;
            node->rect.y = cursor_y;
            node->rect.w = body_right - body_left;
            node->rect.h = h;
            cursor_y += h + 6;
        } else if (node->type == BARAM_UI_NODE_BUTTON) {
            int button_w = win_text_width_px(node->text, 2) + 16;
            int button_h = 24;
            if (button_w < 88) button_w = 88;
            if (button_w > body_right - body_left) button_w = body_right - body_left;
            node->rect.x = body_left;
            node->rect.y = cursor_y;
            node->rect.w = button_w;
            node->rect.h = button_h;
            cursor_y += button_h + 8;
        }
        if (node->rect.y + node->rect.h > ctx->dock_rect.y - 4) {
            node->rect.y = ctx->dock_rect.y - node->rect.h - 4;
        }
    }

    win_add_layer(ctx, BARAM_UI_LAYER_WINDOW_BG, -1, ctx->window_rect.x, ctx->window_rect.y, ctx->window_rect.w, ctx->window_rect.h);
    win_add_layer(ctx, BARAM_UI_LAYER_TITLEBAR, -1, ctx->window_rect.x, ctx->window_rect.y, ctx->window_rect.w, titlebar_h);
    win_add_layer(ctx, BARAM_UI_LAYER_CLOSE, -1, ctx->close_rect.x, ctx->close_rect.y, ctx->close_rect.w, ctx->close_rect.h);

    for (i = 0; i < ctx->node_count; i++) {
        int layer_type = BARAM_UI_LAYER_TEXT;
        if (ctx->nodes[i].type == BARAM_UI_NODE_TITLE) layer_type = BARAM_UI_LAYER_TITLE;
        else if (ctx->nodes[i].type == BARAM_UI_NODE_BUTTON) layer_type = BARAM_UI_LAYER_BUTTON;
        win_add_layer(ctx, layer_type, i, ctx->nodes[i].rect.x, ctx->nodes[i].rect.y, ctx->nodes[i].rect.w, ctx->nodes[i].rect.h);
    }

    win_add_layer(ctx, BARAM_UI_LAYER_DOCK, -1, ctx->dock_rect.x, ctx->dock_rect.y, ctx->dock_rect.w, ctx->dock_rect.h);
    win_add_layer(ctx, BARAM_UI_LAYER_POINTER, -1, 0, 0, ctx->width, ctx->height);
    ctx->needs_redraw = 1;
}
#endif

#ifndef BARAM_UI_EMBEDDED
static int baram_ui_win_streq(const char *a, const char *b) {
    while (*a && *b && *a == *b) {
        a++;
        b++;
    }
    return *a == 0 && *b == 0;
}

int xiao_app_entry(xiao_env *env) {
    const char *argv[8];
    int argc = xiao_argc(env);
    const char *arg1;
    int i;

    if (argc <= 1) {
        argv[0] = "Baram-UI";
        argv[1] = "open";
        argv[2] = "/gui/desktop.html";
        return xiao_exec_app_args("Baram-UI", 3, argv);
    }

    arg1 = xiao_argv(env, 1);
    if (arg1 && (baram_ui_win_streq(arg1, "open") || baram_ui_win_streq(arg1, "navigate") ||
                 baram_ui_win_streq(arg1, "resize") || baram_ui_win_streq(arg1, "set") ||
                 baram_ui_win_streq(arg1, "operate") || baram_ui_win_streq(arg1, "close"))) {
        if (argc > 8) argc = 8;
        argv[0] = "Baram-UI";
        for (i = 1; i < argc; i++) argv[i] = xiao_argv(env, i);
        return xiao_exec_app_args("Baram-UI", argc, argv);
    }

    argv[0] = "Baram-UI";
    argv[1] = "open";
    argv[2] = arg1 ? arg1 : "/gui/desktop.html";
    return xiao_exec_app_args("Baram-UI", 3, argv);
}
#endif
