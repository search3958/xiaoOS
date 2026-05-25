#include "Baram-UI-ABI.h"

#ifdef BARAM_UI_EMBEDDED
#ifndef XIAO_TTF_TERMINAL_DISABLED
static xiao_size shell_strlen(const char *s);
#endif
#ifndef XIAO_TTF_TERMINAL_DISABLED
#define STBTT_STATIC
#ifndef NULL
#define NULL ((void *)0)
#endif
#define BARAM_UI_TTF_ARENA_SIZE (96 * 1024)
#define BARAM_UI_GLYPH_SIDE_MAX 64
#define BARAM_UI_GLYPH_BITMAP_MAX (BARAM_UI_GLYPH_SIDE_MAX * BARAM_UI_GLYPH_SIDE_MAX)

typedef struct {
    unsigned int used;
    unsigned char buf[BARAM_UI_TTF_ARENA_SIZE];
} baram_ui_ttf_arena_state;

static baram_ui_ttf_arena_state baram_ui_ttf_arena;

static void *baram_ui_ttf_malloc(unsigned int size, void *userdata) {
    unsigned int aligned;
    (void)userdata;
    if (size == 0) size = 1;
    aligned = (size + 7u) & ~7u;
    if (baram_ui_ttf_arena.used + aligned > BARAM_UI_TTF_ARENA_SIZE) return 0;
    {
        void *ptr = baram_ui_ttf_arena.buf + baram_ui_ttf_arena.used;
        baram_ui_ttf_arena.used += aligned;
        return ptr;
    }
}

static void baram_ui_ttf_free(void *ptr, void *userdata) {
    (void)ptr;
    (void)userdata;
}

static void baram_ui_ttf_reset_alloc(void) {
    baram_ui_ttf_arena.used = 0;
}

static void *baram_ui_ttf_memcpy(void *dst, const void *src, xiao_size n) {
    xiao_size i;
    unsigned char *d = (unsigned char *)dst;
    const unsigned char *s = (const unsigned char *)src;
    for (i = 0; i < n; i++) d[i] = s[i];
    return dst;
}

static void *baram_ui_ttf_memset(void *dst, int value, xiao_size n) {
    xiao_size i;
    unsigned char *d = (unsigned char *)dst;
    for (i = 0; i < n; i++) d[i] = (unsigned char)value;
    return dst;
}

static float baram_ui_ttf_fabs(float x) { return x < 0.0f ? -x : x; }
static int baram_ui_ttf_ifloor(float x) { int i = (int)x; return (i > x) ? (i - 1) : i; }
static int baram_ui_ttf_iceil(float x) { int i = (int)x; return (i < x) ? (i + 1) : i; }

static float baram_ui_ttf_sqrt(float x) {
    float r;
    int i;
    if (x <= 0.0f) return 0.0f;
    r = x > 1.0f ? x : 1.0f;
    for (i = 0; i < 8; i++) r = 0.5f * (r + x / r);
    return r;
}

static float baram_ui_ttf_fmod(float x, float y) {
    int q;
    if (y == 0.0f) return 0.0f;
    q = (int)(x / y);
    return x - (float)q * y;
}

static float baram_ui_ttf_pow(float x, float y) {
    int e;
    float r = 1.0f;
    if (y == 0.0f) return 1.0f;
    e = (int)y;
    if ((float)e != y || e < 0) return x > 0.0f ? baram_ui_ttf_sqrt(x) : 0.0f;
    while (e > 0) {
        r *= x;
        e--;
    }
    return r;
}

static float baram_ui_ttf_cos(float x) {
    float x2;
    while (x > 3.1415926f) x -= 6.2831852f;
    while (x < -3.1415926f) x += 6.2831852f;
    x2 = x * x;
    return 1.0f - x2 * 0.5f + (x2 * x2) * (1.0f / 24.0f);
}

static float baram_ui_ttf_acos(float x) {
    float y;
    if (x <= -1.0f) return 3.1415926f;
    if (x >= 1.0f) return 0.0f;
    y = baram_ui_ttf_sqrt(1.0f - x * x);
    if (x == 0.0f) return 1.5707963f;
    if (x > 0.0f) return y;
    return 3.1415926f - y;
}

#define STBTT_assert(x) ((void)(x))
#define STBTT_malloc(x,u) baram_ui_ttf_malloc((unsigned int)(x), (u))
#define STBTT_free(x,u) baram_ui_ttf_free((x), (u))
#define STBTT_strlen(x) shell_strlen((x))
#define STBTT_memcpy(d,s,n) baram_ui_ttf_memcpy((d), (s), (n))
#define STBTT_memset(d,v,n) baram_ui_ttf_memset((d), (v), (n))
#define STBTT_ifloor(x) baram_ui_ttf_ifloor((float)(x))
#define STBTT_iceil(x) baram_ui_ttf_iceil((float)(x))
#define STBTT_sqrt(x) baram_ui_ttf_sqrt((float)(x))
#define STBTT_pow(x,y) baram_ui_ttf_pow((float)(x), (float)(y))
#define STBTT_fmod(x,y) baram_ui_ttf_fmod((float)(x), (float)(y))
#define STBTT_cos(x) baram_ui_ttf_cos((float)(x))
#define STBTT_acos(x) baram_ui_ttf_acos((float)(x))
#define STBTT_fabs(x) baram_ui_ttf_fabs((float)(x))
#define STB_TRUETYPE_IMPLEMENTATION
#include "stb_truetype.h"
#endif

static xiao_size shell_strlen(const char *s) {
    xiao_size n = 0;
    while (s && s[n]) n++;
    return n;
}

static void shell_set_pending_action(baram_ui_context *ctx, const char *action);

static void shell_copy_cap(char *dst, xiao_size cap, const char *src) {
    xiao_size i = 0;
    if (!dst || cap == 0) return;
    while (src && src[i] && i + 1 < cap) {
        dst[i] = src[i];
        i++;
    }
    dst[i] = 0;
}

static int shell_streq(const char *a, const char *b) {
    while (*a && *b && *a == *b) {
        a++;
        b++;
    }
    return *a == 0 && *b == 0;
}

static int shell_starts_with(const char *s, const char *prefix) {
    while (*prefix) {
        if (*s != *prefix) return 0;
        s++;
        prefix++;
    }
    return 1;
}

static const char *shell_find_text(const char *haystack, const char *needle) {
    xiao_size nlen = shell_strlen(needle);
    if (!haystack || !needle || nlen == 0) return 0;
    while (*haystack) {
        xiao_size i = 0;
        while (i < nlen && haystack[i] == needle[i]) i++;
        if (i == nlen) return haystack;
        haystack++;
    }
    return 0;
}

static const char *shell_find_char(const char *s, char ch) {
    while (s && *s) {
        if (*s == ch) return s;
        s++;
    }
    return 0;
}

static int shell_point_in_rect(int x, int y, const baram_ui_rect *r) {
    if (!r) return 0;
    if (r->w <= 0 || r->h <= 0) return 0;
    if (x < r->x || y < r->y) return 0;
    if (x >= r->x + r->w || y >= r->y + r->h) return 0;
    return 1;
}

static void shell_fill_rect(baram_ui_context *ctx, int x, int y, int w, int h, unsigned int color) {
    if (w <= 0 || h <= 0) return;
    xiao_video_fill_rect_rgb888(ctx->env, x, y, w, h, color);
}

static void shell_frame_rect(baram_ui_context *ctx, const baram_ui_rect *r, unsigned int color) {
    if (!r || r->w <= 1 || r->h <= 1) return;
    shell_fill_rect(ctx, r->x, r->y, r->w, 1, color);
    shell_fill_rect(ctx, r->x, r->y + r->h - 1, r->w, 1, color);
    shell_fill_rect(ctx, r->x, r->y, 1, r->h, color);
    shell_fill_rect(ctx, r->x + r->w - 1, r->y, 1, r->h, color);
}

static const unsigned char shell_font_upper[26][7] = {
    {0x0E,0x11,0x11,0x1F,0x11,0x11,0x11},
    {0x1E,0x11,0x11,0x1E,0x11,0x11,0x1E},
    {0x0E,0x11,0x10,0x10,0x10,0x11,0x0E},
    {0x1E,0x11,0x11,0x11,0x11,0x11,0x1E},
    {0x1F,0x10,0x10,0x1E,0x10,0x10,0x1F},
    {0x1F,0x10,0x10,0x1E,0x10,0x10,0x10},
    {0x0E,0x11,0x10,0x17,0x11,0x11,0x0F},
    {0x11,0x11,0x11,0x1F,0x11,0x11,0x11},
    {0x0E,0x04,0x04,0x04,0x04,0x04,0x0E},
    {0x01,0x01,0x01,0x01,0x11,0x11,0x0E},
    {0x11,0x12,0x14,0x18,0x14,0x12,0x11},
    {0x10,0x10,0x10,0x10,0x10,0x10,0x1F},
    {0x11,0x1B,0x15,0x15,0x11,0x11,0x11},
    {0x11,0x19,0x15,0x13,0x11,0x11,0x11},
    {0x0E,0x11,0x11,0x11,0x11,0x11,0x0E},
    {0x1E,0x11,0x11,0x1E,0x10,0x10,0x10},
    {0x0E,0x11,0x11,0x11,0x15,0x12,0x0D},
    {0x1E,0x11,0x11,0x1E,0x14,0x12,0x11},
    {0x0F,0x10,0x10,0x0E,0x01,0x01,0x1E},
    {0x1F,0x04,0x04,0x04,0x04,0x04,0x04},
    {0x11,0x11,0x11,0x11,0x11,0x11,0x0E},
    {0x11,0x11,0x11,0x11,0x11,0x0A,0x04},
    {0x11,0x11,0x11,0x15,0x15,0x15,0x0A},
    {0x11,0x11,0x0A,0x04,0x0A,0x11,0x11},
    {0x11,0x11,0x0A,0x04,0x04,0x04,0x04},
    {0x1F,0x01,0x02,0x04,0x08,0x10,0x1F}
};

static const unsigned char shell_font_digit[10][7] = {
    {0x0E,0x11,0x13,0x15,0x19,0x11,0x0E},
    {0x04,0x0C,0x04,0x04,0x04,0x04,0x0E},
    {0x0E,0x11,0x01,0x02,0x04,0x08,0x1F},
    {0x1E,0x01,0x01,0x0E,0x01,0x01,0x1E},
    {0x02,0x06,0x0A,0x12,0x1F,0x02,0x02},
    {0x1F,0x10,0x10,0x1E,0x01,0x01,0x1E},
    {0x0E,0x10,0x10,0x1E,0x11,0x11,0x0E},
    {0x1F,0x01,0x02,0x04,0x08,0x08,0x08},
    {0x0E,0x11,0x11,0x0E,0x11,0x11,0x0E},
    {0x0E,0x11,0x11,0x0F,0x01,0x01,0x0E}
};

static const unsigned char shell_font_qmark[7] = {0x0E,0x11,0x01,0x02,0x04,0x00,0x04};
static const unsigned char shell_font_dot[7] = {0x00,0x00,0x00,0x00,0x00,0x00,0x04};
static const unsigned char shell_font_dash[7] = {0x00,0x00,0x00,0x1F,0x00,0x00,0x00};
static const unsigned char shell_font_colon[7] = {0x00,0x04,0x00,0x00,0x04,0x00,0x00};
static const unsigned char shell_font_slash[7] = {0x01,0x02,0x04,0x08,0x10,0x00,0x00};
static const unsigned char shell_font_underscore[7] = {0x00,0x00,0x00,0x00,0x00,0x00,0x1F};

static const unsigned char *shell_font_for_char(char c) {
    if (c >= 'a' && c <= 'z') c = (char)(c - ('a' - 'A'));
    if (c >= 'A' && c <= 'Z') return shell_font_upper[(int)(c - 'A')];
    if (c >= '0' && c <= '9') return shell_font_digit[(int)(c - '0')];
    if (c == ' ') return 0;
    if (c == '.') return shell_font_dot;
    if (c == '-') return shell_font_dash;
    if (c == ':') return shell_font_colon;
    if (c == '/') return shell_font_slash;
    if (c == '_') return shell_font_underscore;
    return shell_font_qmark;
}

#ifndef XIAO_TTF_TERMINAL_DISABLED
typedef struct {
    int ready;
    stbtt_fontinfo font;
    float body_scale;
    float title_scale;
    int body_ascent;
    int title_ascent;
} baram_ui_ttf_state;

static baram_ui_ttf_state shell_ttf_state;

static int shell_ttf_init(void) {
    const char *font_data = 0;
    xiao_size font_size = 0;
    int ascent = 0;
    int descent = 0;
    int line_gap = 0;

    if (shell_ttf_state.ready) return 0;
    if (xiao_fs_read("/IBMPlexMono-Regular.ttf", &font_data, &font_size) != 0) return -1;
    if (font_size < 1024) return -1;
    if (!stbtt_InitFont(&shell_ttf_state.font, (const unsigned char *)font_data, stbtt_GetFontOffsetForIndex((const unsigned char *)font_data, 0))) {
        return -1;
    }
    shell_ttf_state.body_scale = stbtt_ScaleForPixelHeight(&shell_ttf_state.font, 14.0f);
    shell_ttf_state.title_scale = stbtt_ScaleForPixelHeight(&shell_ttf_state.font, 22.0f);
    stbtt_GetFontVMetrics(&shell_ttf_state.font, &ascent, &descent, &line_gap);
    shell_ttf_state.body_ascent = (int)((float)ascent * shell_ttf_state.body_scale + 0.5f);
    shell_ttf_state.title_ascent = (int)((float)ascent * shell_ttf_state.title_scale + 0.5f);
    shell_ttf_state.ready = 1;
    return 0;
}

static int shell_ttf_advance_px(float scale, int cp, int next_cp) {
    int adv = 0;
    int lsb = 0;
    float px;
    stbtt_GetCodepointHMetrics(&shell_ttf_state.font, cp, &adv, &lsb);
    (void)lsb;
    px = (float)adv * scale;
    if (next_cp > 0) px += (float)stbtt_GetCodepointKernAdvance(&shell_ttf_state.font, cp, next_cp) * scale;
    if (px >= 0.0f) return (int)(px + 0.5f);
    return (int)(px - 0.5f);
}

static unsigned int shell_blend_rgb888(unsigned int bg, unsigned int fg, unsigned char a) {
    unsigned int br = (bg >> 16) & 0xFFu;
    unsigned int bgc = (bg >> 8) & 0xFFu;
    unsigned int bb = bg & 0xFFu;
    unsigned int fr = (fg >> 16) & 0xFFu;
    unsigned int fgc = (fg >> 8) & 0xFFu;
    unsigned int fb = fg & 0xFFu;
    unsigned int ia = 255u - (unsigned int)a;
    unsigned int rr = (fr * (unsigned int)a + br * ia + 127u) / 255u;
    unsigned int rg = (fgc * (unsigned int)a + bgc * ia + 127u) / 255u;
    unsigned int rb = (fb * (unsigned int)a + bb * ia + 127u) / 255u;
    return (rr << 16) | (rg << 8) | rb;
}

static int shell_ttf_draw_glyph(baram_ui_context *ctx, int pen_x, int baseline_y, int cp, int next_cp, float scale, unsigned int fg, unsigned int bg) {
    static unsigned char bitmap[BARAM_UI_GLYPH_BITMAP_MAX];
    int x0, y0, x1, y1;
    int w, h;
    int gx, gy;

    if (cp < 32 || cp > 126) cp = '?';
    stbtt_GetCodepointBitmapBoxSubpixel(&shell_ttf_state.font, cp, scale, scale, 0.0f, 0.0f, &x0, &y0, &x1, &y1);
    w = x1 - x0;
    h = y1 - y0;
    if (w > 0 && h > 0 && w <= BARAM_UI_GLYPH_SIDE_MAX && h <= BARAM_UI_GLYPH_SIDE_MAX && w * h <= BARAM_UI_GLYPH_BITMAP_MAX) {
        baram_ui_ttf_reset_alloc();
        stbtt_MakeCodepointBitmapSubpixel(&shell_ttf_state.font, bitmap, w, h, w, scale, scale, 0.0f, 0.0f, cp);
        for (gy = 0; gy < h; gy++) {
            for (gx = 0; gx < w; gx++) {
                unsigned char a = bitmap[gy * w + gx];
                if (a == 0) continue;
                if (a >= 250) xiao_video_draw_pixel_rgb888(ctx->env, pen_x + x0 + gx, baseline_y + y0 + gy, fg);
                else xiao_video_draw_pixel_rgb888(ctx->env, pen_x + x0 + gx, baseline_y + y0 + gy, shell_blend_rgb888(bg, fg, a));
            }
        }
    }
    pen_x += shell_ttf_advance_px(scale, cp, next_cp);
    return pen_x;
}
#endif

static void shell_draw_char(baram_ui_context *ctx, int x, int y, char c, int scale, unsigned int color) {
    const unsigned char *glyph = shell_font_for_char(c);
    int row;
    if (!glyph || scale < 1) return;
    for (row = 0; row < 7; row++) {
        unsigned char bits = glyph[row];
        int col;
        for (col = 0; col < 5; col++) {
            if (bits & (1u << (4 - col))) {
                shell_fill_rect(ctx, x + col * scale, y + row * scale, scale, scale, color);
            }
        }
    }
}

static void shell_draw_text(baram_ui_context *ctx, int x, int y, const char *text, int scale, unsigned int color, unsigned int bg_color, int max_x) {
#ifndef XIAO_TTF_TERMINAL_DISABLED
    if (shell_ttf_init() == 0) {
        float s = scale > 1 ? shell_ttf_state.title_scale : shell_ttf_state.body_scale;
        int ascent = scale > 1 ? shell_ttf_state.title_ascent : shell_ttf_state.body_ascent;
        int pen_x = x;
        int baseline = y + ascent;
        xiao_size i = 0;
        while (text && text[i]) {
            int cp = (unsigned char)text[i];
            int next = text[i + 1] ? (unsigned char)text[i + 1] : 0;
            if (max_x > 0 && pen_x >= max_x) break;
            pen_x = shell_ttf_draw_glyph(ctx, pen_x, baseline, cp, next, s, color, bg_color);
            i++;
        }
        return;
    }
#endif
    int pen_x = x;
    int step;
    xiao_size i = 0;
    if (!text) return;
    if (scale < 1) scale = 1;
    step = 6 * scale;
    while (text[i]) {
        if (max_x > 0 && pen_x + step > max_x) break;
        shell_draw_char(ctx, pen_x, y, text[i], scale, color);
        pen_x += step;
        i++;
    }
}

static int shell_app_exists(const char *name) {
    xiao_size i;
    for (i = 0; i < xiao_app_count(); i++) {
        const char *n = xiao_app_name(i);
        if (n && shell_streq(n, name)) return 1;
    }
    return 0;
}

static int shell_collect_dock_apps(const char **out, int cap) {
    static const char *fallback[] = {"gui-desktop", "gui-home", "gui-second", "hello", "terminal", "help"};
    static char dock_names[6][32];
    int n = 0;
    xiao_size i;
    int j;

    if (!out || cap <= 0) return 0;

    {
        const char *dock_data = 0;
        xiao_size dock_size = 0;
        if (xiao_fs_read("/gui/dock.html", &dock_data, &dock_size) == 0 && dock_data && dock_size > 0) {
            const char *p = dock_data;
            while (n < cap) {
                const char *tag = shell_find_text(p, "<app");
                const char *end;
                const char *name_attr;
                const char *q;
                const char *qend;
                xiao_size k = 0;
                if (!tag) break;
                end = shell_find_char(tag, '>');
                if (!end) break;
                name_attr = shell_find_text(tag, "name=\"");
                if (name_attr && name_attr < end) {
                    name_attr += 6;
                    q = name_attr;
                    qend = shell_find_char(q, '"');
                    if (qend && qend <= end) {
                        while (q < qend && k + 1 < sizeof(dock_names[0])) {
                            dock_names[n][k++] = *q++;
                        }
                        dock_names[n][k] = 0;
                        if (dock_names[n][0] && shell_app_exists(dock_names[n])) {
                            out[n] = dock_names[n];
                            n++;
                        }
                    }
                }
                p = end + 1;
            }
        }
    }

    for (i = 0; i < xiao_app_count() && n < cap; i++) {
        const char *name = xiao_app_name(i);
        int k;
        int dup = 0;
        if (!name || !shell_starts_with(name, "gui-")) continue;
        for (k = 0; k < n; k++) {
            if (shell_streq(out[k], name)) {
                dup = 1;
                break;
            }
        }
        if (!dup) out[n++] = name;
    }

    for (j = 0; j < (int)(sizeof(fallback) / sizeof(fallback[0])) && n < cap; j++) {
        int k;
        int duplicate = 0;
        if (!shell_app_exists(fallback[j])) continue;
        for (k = 0; k < n; k++) {
            if (shell_streq(out[k], fallback[j])) {
                duplicate = 1;
                break;
            }
        }
        if (!duplicate) out[n++] = fallback[j];
    }

    return n;
}

static int shell_dock_item_rect(const baram_ui_context *ctx, int idx, int count, baram_ui_rect *out) {
    int cell_w;
    if (!ctx || !out || count <= 0 || idx < 0 || idx >= count) return -1;
    cell_w = ctx->dock_rect.w / count;
    if (cell_w < 36) cell_w = 36;
    out->x = ctx->dock_rect.x + idx * cell_w + 2;
    out->y = ctx->dock_rect.y + 2;
    out->w = cell_w - 4;
    out->h = ctx->dock_rect.h - 4;
    return 0;
}

static int shell_find_hover_dock(baram_ui_context *ctx) {
    const char *apps[6];
    int count = shell_collect_dock_apps(apps, 6);
    int i;
    baram_ui_rect item;
    if (count <= 0) return -1;
    if (!shell_point_in_rect(ctx->pointer_x, ctx->pointer_y, &ctx->dock_rect)) return -1;
    for (i = 0; i < count; i++) {
        shell_dock_item_rect(ctx, i, count, &item);
        if (shell_point_in_rect(ctx->pointer_x, ctx->pointer_y, &item)) return i;
    }
    return -1;
}

static void shell_set_pending_cmd(baram_ui_context *ctx, const char *name) {
    char action[BARAM_UI_ACTION_MAX];
    int n = 0;
    int i = 0;
    if (!ctx || !name || !name[0]) return;
    action[n++] = 'c';
    action[n++] = 'm';
    action[n++] = 'd';
    action[n++] = ':';
    while (name[i] && n + 1 < (int)sizeof(action)) {
        action[n++] = name[i++];
    }
    action[n] = 0;
    shell_set_pending_action(ctx, action);
}

static int shell_find_hover_button(baram_ui_context *ctx) {
    int i;
    for (i = 0; i < ctx->node_count; i++) {
        if (ctx->nodes[i].type != BARAM_UI_NODE_BUTTON) continue;
        if (shell_point_in_rect(ctx->pointer_x, ctx->pointer_y, &ctx->nodes[i].rect)) return i;
    }
    return -1;
}

static void shell_set_pending_action(baram_ui_context *ctx, const char *action) {
    if (!ctx || !action || !action[0]) return;
    if (ctx->pending_action[0]) return;
    shell_copy_cap(ctx->pending_action, sizeof(ctx->pending_action), action);
}

void baram_ui_shell_invalidate_all(baram_ui_context *ctx) {
    int i;
    if (!ctx) return;
    ctx->needs_redraw = 1;
    for (i = 0; i < ctx->layer_count; i++) {
        ctx->layers[i].dirty = 1;
    }
}

static void shell_render_layer(baram_ui_context *ctx, baram_ui_layer *layer) {
    static const unsigned int desktop_color = 0x24314Bu;
    static const unsigned int window_color = 0xF2F6FCu;
    static const unsigned int titlebar_color = 0x3D4E72u;
    static const unsigned int titlebar_text = 0xFFFFFFu;
    static const unsigned int text_color = 0x1D2736u;
    static const unsigned int button_color = 0xD8E4F2u;
    static const unsigned int button_hover = 0xC0D5EDu;
    static const unsigned int button_press = 0xA4BFE0u;
    static const unsigned int close_color = 0xE26A6Au;
    static const unsigned int close_hover = 0xF29B9Bu;

    if (!ctx || !layer) return;

    if (layer->type == BARAM_UI_LAYER_DESKTOP) {
        shell_fill_rect(ctx, layer->rect.x, layer->rect.y, layer->rect.w, layer->rect.h, desktop_color);
        return;
    }

    if (layer->type == BARAM_UI_LAYER_WINDOW_BG) {
        shell_fill_rect(ctx, layer->rect.x, layer->rect.y, layer->rect.w, layer->rect.h, window_color);
        shell_frame_rect(ctx, &layer->rect, 0x8EA3C2u);
        return;
    }

    if (layer->type == BARAM_UI_LAYER_TITLEBAR) {
        shell_fill_rect(ctx, layer->rect.x, layer->rect.y, layer->rect.w, layer->rect.h, titlebar_color);
        shell_draw_text(ctx, layer->rect.x + 8, layer->rect.y + 6, ctx->window_title, 1, titlebar_text, titlebar_color, layer->rect.x + layer->rect.w - 28);
        return;
    }

    if (layer->type == BARAM_UI_LAYER_CLOSE) {
        int hover_close = shell_point_in_rect(ctx->pointer_x, ctx->pointer_y, &ctx->close_rect);
        unsigned int fill = hover_close ? close_hover : close_color;
        shell_fill_rect(ctx, layer->rect.x, layer->rect.y, layer->rect.w, layer->rect.h, fill);
        shell_draw_text(ctx, layer->rect.x + 5, layer->rect.y + 4, "X", 1, 0xFFFFFFu, fill, layer->rect.x + layer->rect.w);
        return;
    }

    if (layer->type == BARAM_UI_LAYER_DOCK) {
        const char *apps[6];
        int count = shell_collect_dock_apps(apps, 6);
        int i;
        baram_ui_rect item;
        shell_fill_rect(ctx, layer->rect.x, layer->rect.y, layer->rect.w, layer->rect.h, 0x2A3855u);
        shell_frame_rect(ctx, &layer->rect, 0x51658Du);
        for (i = 0; i < count; i++) {
            unsigned int fill = 0xB9C7DEu;
            shell_dock_item_rect(ctx, i, count, &item);
            if (ctx->pressed_dock == i) fill = 0x8FA7C8u;
            else if (ctx->hovered_dock == i) fill = 0xA8BCD8u;
            shell_fill_rect(ctx, item.x, item.y, item.w, item.h, fill);
            shell_frame_rect(ctx, &item, 0x627BA0u);
            shell_draw_text(ctx, item.x + 4, item.y + 8, apps[i], 1, 0x17202Eu, fill, item.x + item.w - 2);
        }
        return;
    }

    if (layer->node_index >= 0 && layer->node_index < ctx->node_count) {
        baram_ui_node *node = &ctx->nodes[layer->node_index];

        if (layer->type == BARAM_UI_LAYER_TITLE) {
            shell_draw_text(ctx, node->rect.x, node->rect.y, node->text, 2, 0x1F2E42u, window_color, node->rect.x + node->rect.w);
            return;
        }

        if (layer->type == BARAM_UI_LAYER_TEXT) {
            shell_draw_text(ctx, node->rect.x, node->rect.y, node->text, 1, text_color, window_color, node->rect.x + node->rect.w);
            return;
        }

        if (layer->type == BARAM_UI_LAYER_BUTTON) {
            unsigned int fill = button_color;
            if (ctx->pressed_button == layer->node_index) fill = button_press;
            else if (ctx->hovered_button == layer->node_index) fill = button_hover;
            shell_fill_rect(ctx, node->rect.x, node->rect.y, node->rect.w, node->rect.h, fill);
            shell_frame_rect(ctx, &node->rect, 0x6E86A6u);
            shell_draw_text(ctx, node->rect.x + 6, node->rect.y + 8, node->text, 1, 0x17202Eu, fill, node->rect.x + node->rect.w - 4);
            return;
        }
    }

    if (layer->type == BARAM_UI_LAYER_POINTER) {
        baram_ui_pointer_draw(ctx);
    }
}

static int shell_update_interaction(baram_ui_context *ctx) {
    int prev_hover = ctx->hovered_button;
    int prev_pressed = ctx->pressed_button;
    int prev_hover_dock = ctx->hovered_dock;
    int prev_pressed_dock = ctx->pressed_dock;
    int hover;
    int hover_dock;
    int prev_left;
    int now_left;
    int pointer_moved;

    hover = shell_find_hover_button(ctx);
    hover_dock = shell_find_hover_dock(ctx);
    ctx->hovered_button = hover;
    ctx->hovered_dock = hover_dock;

    prev_left = (ctx->pointer_prev_buttons & 1) != 0;
    now_left = (ctx->pointer_buttons & 1) != 0;
    pointer_moved = (ctx->pointer_prev_x != ctx->pointer_x) || (ctx->pointer_prev_y != ctx->pointer_y);

    if (!prev_left && now_left) {
        if (shell_point_in_rect(ctx->pointer_x, ctx->pointer_y, &ctx->close_rect)) {
            shell_set_pending_action(ctx, "close");
        } else if (hover_dock >= 0) {
            ctx->pressed_dock = hover_dock;
            ctx->pressed_button = -1;
        } else {
            ctx->pressed_button = hover;
            ctx->pressed_dock = -1;
        }
    } else if (prev_left && !now_left) {
        if (ctx->pressed_button >= 0 && ctx->pressed_button == hover) {
            shell_set_pending_action(ctx, ctx->nodes[ctx->pressed_button].action);
        }
        if (ctx->pressed_dock >= 0 && ctx->pressed_dock == hover_dock) {
            const char *apps[6];
            int count = shell_collect_dock_apps(apps, 6);
            if (ctx->pressed_dock < count) shell_set_pending_cmd(ctx, apps[ctx->pressed_dock]);
        }
        ctx->pressed_button = -1;
        ctx->pressed_dock = -1;
    }

    if (pointer_moved ||
        prev_hover != ctx->hovered_button ||
        prev_pressed != ctx->pressed_button ||
        prev_hover_dock != ctx->hovered_dock ||
        prev_pressed_dock != ctx->pressed_dock) {
        return 1;
    }
    return 0;
}

int baram_ui_shell_tick(baram_ui_context *ctx) {
    int changed;
    int i;

    if (!ctx || !ctx->env || !ctx->video_ready) return 0;

    changed = shell_update_interaction(ctx);
    if (changed) baram_ui_shell_invalidate_all(ctx);

    if (!ctx->needs_redraw) return 0;

    for (i = 0; i < ctx->layer_count; i++) {
        if (ctx->needs_redraw || ctx->layers[i].dirty) {
            shell_render_layer(ctx, &ctx->layers[i]);
            ctx->layers[i].dirty = 0;
        }
    }
    ctx->needs_redraw = 0;
    return 1;
}

int baram_ui_shell_consume_action(baram_ui_context *ctx, char *out, xiao_size cap) {
    if (!ctx || !out || cap == 0) return 0;
    if (!ctx->pending_action[0]) {
        out[0] = 0;
        return 0;
    }
    shell_copy_cap(out, cap, ctx->pending_action);
    ctx->pending_action[0] = 0;
    return 1;
}
#endif

#ifndef BARAM_UI_EMBEDDED
int xiao_app_entry(xiao_env *env) {
    const char *argv[3];
    int argc = xiao_argc(env);

    argv[0] = "Baram-UI";
    argv[1] = "open";
    if (argc >= 2 && xiao_argv(env, 1)) {
        argv[2] = xiao_argv(env, 1);
    } else {
        argv[2] = "/gui/desktop.html";
    }

    return xiao_exec_app_args("Baram-UI", 3, argv);
}
#endif
