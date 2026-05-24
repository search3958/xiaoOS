#include "xiao.h"

#ifndef XIAO_TTF_TERMINAL_DISABLED
#define STBTT_STATIC
#ifndef NULL
#define NULL ((void *)0)
#endif

#define UI_GLYPH_SIDE_MAX 96
#define UI_GLYPH_BITMAP_MAX (UI_GLYPH_SIDE_MAX * UI_GLYPH_SIDE_MAX)
#define UI_TTF_ARENA_SIZE (128 * 1024)
#define UI_LAYER_COUNT 10
#define UI_CURSOR_SIZE 7

enum {
    LAYER_ROOT_BG = 0,
    LAYER_PANEL_BG = 1,
    LAYER_PANEL_ACCENT = 2,
    LAYER_TITLE_TEXT = 3,
    LAYER_PARAGRAPH_TEXT = 4,
    LAYER_HINT_TEXT = 5,
    LAYER_BUTTON_BG = 6,
    LAYER_BUTTON_TEXT = 7,
    LAYER_STATUS_TEXT = 8,
    LAYER_CURSOR = 9
};

typedef struct {
    xiao_env *env;
    int width;
    int height;
} hagl_surface_t;

typedef struct {
    unsigned int bg_color;
    unsigned int panel_color;
    unsigned int text_color;
    unsigned int muted_text_color;
    unsigned int accent_color;
    unsigned int button_color;
    unsigned int button_hover_color;
    unsigned int button_pressed_color;
    unsigned int button_text_color;
    int panel_margin;
    int panel_padding;
    int title_px;
    int body_px;
    int button_height;
} litehtml_style_t;

typedef struct {
    char title[96];
    char paragraph[160];
    char button[64];
    litehtml_style_t style;
} litehtml_document_t;

typedef struct {
    int panel_x;
    int panel_y;
    int panel_w;
    int panel_h;
    int title_x;
    int title_y;
    int paragraph_x;
    int paragraph_y;
    int hint_x;
    int hint_y;
    int button_x;
    int button_y;
    int button_w;
    int button_h;
    int button_text_x;
    int button_text_y;
    int status_x;
    int status_y;
    int line_step;
} litehtml_layout_t;

static xiao_size xstrlen(const char *s) {
    xiao_size n = 0;
    while (s && s[n]) n++;
    return n;
}

#if defined(ARDUINO)
/* In Arduino sketch builds, terminal.c already provides stb_truetype implementation. */
static void uitest_ttf_reset_alloc(void) {
    /* shared stb allocator reset is not exported in sketch mode */
}
#else
typedef struct {
    unsigned int used;
    unsigned char buf[UI_TTF_ARENA_SIZE];
} uitest_ttf_arena_state;

static uitest_ttf_arena_state uitest_ttf_arena;

static void *uitest_ttf_malloc(unsigned int size, void *userdata) {
    unsigned int aligned;
    (void)userdata;
    if (size == 0) size = 1;
    aligned = (size + 7u) & ~7u;
    if (uitest_ttf_arena.used + aligned > UI_TTF_ARENA_SIZE) return 0;
    {
        void *ptr = uitest_ttf_arena.buf + uitest_ttf_arena.used;
        uitest_ttf_arena.used += aligned;
        return ptr;
    }
}

static void uitest_ttf_free(void *ptr, void *userdata) {
    (void)ptr;
    (void)userdata;
}

static void uitest_ttf_reset_alloc(void) {
    uitest_ttf_arena.used = 0;
}

static void *uitest_ttf_memcpy(void *dst, const void *src, xiao_size n) {
    xiao_size i;
    unsigned char *d = (unsigned char *)dst;
    const unsigned char *s = (const unsigned char *)src;
    for (i = 0; i < n; i++) d[i] = s[i];
    return dst;
}

static void *uitest_ttf_memset(void *dst, int value, xiao_size n) {
    xiao_size i;
    unsigned char *d = (unsigned char *)dst;
    for (i = 0; i < n; i++) d[i] = (unsigned char)value;
    return dst;
}

static float uitest_ttf_fabs(float x) {
    return x < 0.0f ? -x : x;
}

static int uitest_ttf_ifloor(float x) {
    int i = (int)x;
    return (i > x) ? (i - 1) : i;
}

static int uitest_ttf_iceil(float x) {
    int i = (int)x;
    return (i < x) ? (i + 1) : i;
}

static float uitest_ttf_sqrt(float x) {
    float r;
    int i;
    if (x <= 0.0f) return 0.0f;
    r = x > 1.0f ? x : 1.0f;
    for (i = 0; i < 8; i++) {
        r = 0.5f * (r + x / r);
    }
    return r;
}

static float uitest_ttf_fmod(float x, float y) {
    int q;
    if (y == 0.0f) return 0.0f;
    q = (int)(x / y);
    return x - (float)q * y;
}

static float uitest_ttf_pow(float x, float y) {
    int e;
    float r = 1.0f;
    if (y == 0.0f) return 1.0f;
    e = (int)y;
    if ((float)e != y || e < 0) {
        return x > 0.0f ? uitest_ttf_sqrt(x) : 0.0f;
    }
    while (e > 0) {
        r *= x;
        e--;
    }
    return r;
}

static float uitest_ttf_cos(float x) {
    float x2;
    while (x > 3.1415926f) x -= 6.2831852f;
    while (x < -3.1415926f) x += 6.2831852f;
    x2 = x * x;
    return 1.0f - x2 * 0.5f + (x2 * x2) * (1.0f / 24.0f);
}

static float uitest_ttf_acos(float x) {
    float y;
    if (x <= -1.0f) return 3.1415926f;
    if (x >= 1.0f) return 0.0f;
    y = uitest_ttf_sqrt(1.0f - x * x);
    if (x == 0.0f) return 1.5707963f;
    if (x > 0.0f) return y;
    return 3.1415926f - y;
}

#define STBTT_assert(x) ((void)(x))
#define STBTT_malloc(x,u) uitest_ttf_malloc((unsigned int)(x), (u))
#define STBTT_free(x,u) uitest_ttf_free((x), (u))
#define STBTT_strlen(x) xstrlen((x))
#define STBTT_memcpy(d,s,n) uitest_ttf_memcpy((d), (s), (n))
#define STBTT_memset(d,v,n) uitest_ttf_memset((d), (v), (n))
#define STBTT_ifloor(x) uitest_ttf_ifloor((float)(x))
#define STBTT_iceil(x) uitest_ttf_iceil((float)(x))
#define STBTT_sqrt(x) uitest_ttf_sqrt((float)(x))
#define STBTT_pow(x,y) uitest_ttf_pow((float)(x), (float)(y))
#define STBTT_fmod(x,y) uitest_ttf_fmod((float)(x), (float)(y))
#define STBTT_cos(x) uitest_ttf_cos((float)(x))
#define STBTT_acos(x) uitest_ttf_acos((float)(x))
#define STBTT_fabs(x) uitest_ttf_fabs((float)(x))
#define STB_TRUETYPE_IMPLEMENTATION
#include "stb_truetype.h"
#endif

typedef struct {
    stbtt_fontinfo font;
    float title_scale;
    float body_scale;
    int title_ascent;
    int body_ascent;
    int title_line_h;
    int body_line_h;
} ui_ttf_font_t;

typedef struct ui_state ui_state_t;
typedef void (*layer_render_fn_t)(ui_state_t *ui, int layer_id);

typedef struct {
    int x;
    int y;
    int w;
    int h;
    int dirty;
    layer_render_fn_t render;
} ui_layer_t;

struct ui_state {
    hagl_surface_t surface;
    litehtml_document_t doc;
    litehtml_layout_t layout;
    ui_ttf_font_t font;
    ui_layer_t layers[UI_LAYER_COUNT];

    int hovered;
    int pressing;
    int toggled;

    int pointer_supported;
    int pointer_x;
    int pointer_y;
    int pointer_buttons;
    int pointer_prev_x;
    int pointer_prev_y;
    int pointer_prev_buttons;
    int pointer_inited;
};

static void copy_cap(char *dst, xiao_size cap, const char *src) {
    xiao_size i = 0;
    if (!cap) return;
    while (src && src[i] && i + 1 < cap) {
        dst[i] = src[i];
        i++;
    }
    dst[i] = 0;
}

static int starts_with(const char *s, const char *prefix) {
    while (*prefix) {
        if (*s != *prefix) return 0;
        s++;
        prefix++;
    }
    return 1;
}

static const char *find_text(const char *haystack, const char *needle) {
    xiao_size nlen = xstrlen(needle);
    if (!haystack || !needle || nlen == 0) return 0;
    while (*haystack) {
        if (starts_with(haystack, needle)) return haystack;
        haystack++;
    }
    return 0;
}

static int is_space_char(char c) {
    return c == ' ' || c == '\t' || c == '\r' || c == '\n';
}

static void extract_tag_text(const char *html, const char *tag, char *out, xiao_size cap, const char *fallback) {
    char open[24];
    char close[28];
    const char *p;
    const char *start;
    const char *end;
    xiao_size i = 0;
    xiao_size o = 0;
    int prev_space = 0;

    if (!cap) return;
    out[0] = 0;

    open[i++] = '<';
    while (*tag && i + 1 < sizeof(open)) open[i++] = *tag++;
    open[i] = 0;

    i = 0;
    close[i++] = '<';
    close[i++] = '/';
    tag = open + 1;
    while (*tag && i + 1 < sizeof(close)) close[i++] = *tag++;
    close[i++] = '>';
    close[i] = 0;

    p = find_text(html, open);
    if (!p) {
        copy_cap(out, cap, fallback);
        return;
    }

    start = p;
    while (*start && *start != '>') start++;
    if (*start != '>') {
        copy_cap(out, cap, fallback);
        return;
    }
    start++;

    end = find_text(start, close);
    if (!end) {
        copy_cap(out, cap, fallback);
        return;
    }

    while (start < end && o + 1 < cap) {
        char c = *start++;
        if (is_space_char(c)) {
            if (!prev_space && o > 0 && o + 1 < cap) out[o++] = ' ';
            prev_space = 1;
            continue;
        }
        out[o++] = c;
        prev_space = 0;
    }

    while (o > 0 && out[o - 1] == ' ') o--;
    out[o] = 0;

    if (o == 0) copy_cap(out, cap, fallback);
}

static void extract_style_block(const char *html, char *out, xiao_size cap) {
    const char *open = find_text(html, "<style>");
    const char *end;
    xiao_size n = 0;
    if (!cap) return;
    out[0] = 0;
    if (!open) return;
    open += 7;
    end = find_text(open, "</style>");
    if (!end) return;
    while (open < end && n + 1 < cap) {
        out[n++] = *open++;
    }
    out[n] = 0;
}

static int hex_nibble(char c) {
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'a' && c <= 'f') return c - 'a' + 10;
    if (c >= 'A' && c <= 'F') return c - 'A' + 10;
    return -1;
}

static int parse_hex6(const char *s, unsigned int *out) {
    int i;
    unsigned int v = 0;
    for (i = 0; i < 6; i++) {
        int n = hex_nibble(s[i]);
        if (n < 0) return 0;
        v = (v << 4) | (unsigned int)n;
    }
    *out = v;
    return 1;
}

static const char *find_css_value(const char *css, const char *name) {
    const char *p = find_text(css, name);
    if (!p) return 0;
    p += xstrlen(name);
    while (*p && *p != ':') p++;
    if (*p != ':') return 0;
    p++;
    while (*p && is_space_char(*p)) p++;
    return p;
}

static int css_read_color_hex(const char *css, const char *name, unsigned int *out) {
    const char *p = find_css_value(css, name);
    if (!p || *p != '#') return 0;
    p++;
    return parse_hex6(p, out);
}

static int css_read_int_px(const char *css, const char *name, int *out) {
    const char *p = find_css_value(css, name);
    int v = 0;
    int has_digit = 0;
    if (!p) return 0;
    while (*p >= '0' && *p <= '9') {
        has_digit = 1;
        v = v * 10 + (*p - '0');
        p++;
    }
    if (!has_digit) return 0;
    if (!starts_with(p, "px")) return 0;
    *out = v;
    return 1;
}

static void litehtml_default_style(litehtml_style_t *s) {
    s->bg_color = 0xE2E8F0u;
    s->panel_color = 0xFFFFFFu;
    s->text_color = 0x0F172Au;
    s->muted_text_color = 0x475569u;
    s->accent_color = 0x2563EBu;
    s->button_color = 0x2563EBu;
    s->button_hover_color = 0x1D4ED8u;
    s->button_pressed_color = 0x0F766Eu;
    s->button_text_color = 0xF8FAFCu;
    s->panel_margin = 10;
    s->panel_padding = 12;
    s->title_px = 26;
    s->body_px = 17;
    s->button_height = 34;
}

static void litehtml_parse_style(const char *css, litehtml_style_t *style) {
    unsigned int color = 0;
    int px = 0;

    litehtml_default_style(style);
    if (!css || !css[0]) return;

    if (css_read_color_hex(css, "--bg", &color)) style->bg_color = color;
    if (css_read_color_hex(css, "--panel", &color)) style->panel_color = color;
    if (css_read_color_hex(css, "--text", &color)) style->text_color = color;
    if (css_read_color_hex(css, "--muted", &color)) style->muted_text_color = color;
    if (css_read_color_hex(css, "--accent", &color)) style->accent_color = color;
    if (css_read_color_hex(css, "--button", &color)) style->button_color = color;
    if (css_read_color_hex(css, "--button-hover", &color)) style->button_hover_color = color;
    if (css_read_color_hex(css, "--button-pressed", &color)) style->button_pressed_color = color;
    if (css_read_color_hex(css, "--button-text", &color)) style->button_text_color = color;

    if (css_read_int_px(css, "--panel-margin", &px)) style->panel_margin = px;
    if (css_read_int_px(css, "--panel-padding", &px)) style->panel_padding = px;
    if (css_read_int_px(css, "--title-px", &px)) style->title_px = px;
    if (css_read_int_px(css, "--body-px", &px)) style->body_px = px;
    if (css_read_int_px(css, "--button-height", &px)) style->button_height = px;

    if (style->panel_margin < 4) style->panel_margin = 4;
    if (style->panel_padding < 6) style->panel_padding = 6;
    if (style->title_px < 12) style->title_px = 12;
    if (style->body_px < 10) style->body_px = 10;
    if (style->button_height < 20) style->button_height = 20;
}

static void litehtml_parse_demo(const char *html, litehtml_document_t *doc) {
    char css[512];
    if (!doc) return;
    extract_tag_text(html, "h1", doc->title, sizeof(doc->title), "HAGL + LiteHTML UI Test");
    extract_tag_text(html, "p", doc->paragraph, sizeof(doc->paragraph), "Layout by LiteHTML, render by HAGL layers");
    extract_tag_text(html, "button", doc->button, sizeof(doc->button), "Toggle state");
    extract_style_block(html, css, sizeof(css));
    litehtml_parse_style(css, &doc->style);
}

static int hagl_init_surface(hagl_surface_t *s, xiao_env *env) {
    if (!s || !env) return -1;
    s->env = env;
    if (xiao_video_size(env, &s->width, &s->height) != 0) return -1;
    return 0;
}

static void hagl_fill(hagl_surface_t *s, unsigned int color) {
    xiao_video_fill_rgb888(s->env, color);
}

static void hagl_fill_rect(hagl_surface_t *s, int x, int y, int w, int h, unsigned int color) {
    xiao_video_fill_rect_rgb888(s->env, x, y, w, h, color);
}

static int font_advance_px(const ui_ttf_font_t *font, float scale, int cp, int next_cp) {
    int adv = 0;
    int lsb = 0;
    float px;
    stbtt_GetCodepointHMetrics(&font->font, cp, &adv, &lsb);
    (void)lsb;
    px = (float)adv * scale;
    if (next_cp > 0) px += (float)stbtt_GetCodepointKernAdvance(&font->font, cp, next_cp) * scale;
    if (px >= 0.0f) return (int)(px + 0.5f);
    return (int)(px - 0.5f);
}

static int text_width_px(const ui_ttf_font_t *font, float scale, const char *text) {
    int width = 0;
    int i = 0;
    while (text && text[i]) {
        int cp = (unsigned char)text[i];
        int next = text[i + 1] ? (unsigned char)text[i + 1] : 0;
        if (cp < 32 || cp > 126) cp = '?';
        width += font_advance_px(font, scale, cp, next);
        i++;
    }
    return width;
}

static unsigned int blend_rgb888(unsigned int bg, unsigned int fg, unsigned char a) {
    unsigned int ia = (unsigned int)(255 - a);
    unsigned int br = (bg >> 16) & 0xffu;
    unsigned int bgc = (bg >> 8) & 0xffu;
    unsigned int bb = bg & 0xffu;
    unsigned int fr = (fg >> 16) & 0xffu;
    unsigned int fgc = (fg >> 8) & 0xffu;
    unsigned int fb = fg & 0xffu;
    unsigned int r = (fr * (unsigned int)a + br * ia) / 255u;
    unsigned int g = (fgc * (unsigned int)a + bgc * ia) / 255u;
    unsigned int b = (fb * (unsigned int)a + bb * ia) / 255u;
    return (r << 16) | (g << 8) | b;
}

static int draw_glyph(ui_state_t *ui, int pen_x, int baseline_y, int cp, int next_cp, float scale, unsigned int fg, unsigned int bg) {
    static unsigned char bitmap[UI_GLYPH_BITMAP_MAX];
    static unsigned int rowbuf[UI_GLYPH_SIDE_MAX];
    int x0, y0, x1, y1;
    int w, h;
    int gx, gy;

    if (cp < 32 || cp > 126) cp = '?';

    stbtt_GetCodepointBitmapBoxSubpixel(&ui->font.font, cp, scale, scale, 0.0f, 0.0f, &x0, &y0, &x1, &y1);
    w = x1 - x0;
    h = y1 - y0;

    if (w > 0 && h > 0 && w <= UI_GLYPH_SIDE_MAX && h <= UI_GLYPH_SIDE_MAX && w * h <= UI_GLYPH_BITMAP_MAX) {
        uitest_ttf_reset_alloc();
        stbtt_MakeCodepointBitmapSubpixel(&ui->font.font, bitmap, w, h, w, scale, scale, 0.0f, 0.0f, cp);

        for (gy = 0; gy < h; gy++) {
            for (gx = 0; gx < w; gx++) {
                rowbuf[gx] = blend_rgb888(bg, fg, bitmap[gy * w + gx]);
            }
            xiao_video_blit_rgb888(ui->surface.env, pen_x + x0, baseline_y + y0 + gy, w, 1, rowbuf, w);
        }
    }

    pen_x += font_advance_px(&ui->font, scale, cp, next_cp);
    return pen_x;
}

static void draw_text(ui_state_t *ui, int x, int y, const char *text, float scale, int ascent, unsigned int fg, unsigned int bg) {
    int pen_x = x;
    int baseline = y + ascent;
    int i = 0;
    while (text && text[i]) {
        int cp = (unsigned char)text[i];
        int next = text[i + 1] ? (unsigned char)text[i + 1] : 0;
        pen_x = draw_glyph(ui, pen_x, baseline, cp, next, scale, fg, bg);
        i++;
        if (pen_x > ui->surface.width) break;
    }
}

static int ui_font_init(ui_state_t *ui) {
    const char *font_data = 0;
    xiao_size font_size = 0;
    int ascent = 0;
    int descent = 0;
    int line_gap = 0;

    if (xiao_fs_read("/IBMPlexMono-Regular.ttf", &font_data, &font_size) != 0) {
        xiao_console_print(ui->surface.env, "uitest: /IBMPlexMono-Regular.ttf not found\r\n");
        return -1;
    }
    if (font_size < 1024) {
        xiao_console_print(ui->surface.env, "uitest: font data is invalid\r\n");
        return -1;
    }
    if (!stbtt_InitFont(&ui->font.font, (const unsigned char *)font_data, stbtt_GetFontOffsetForIndex((const unsigned char *)font_data, 0))) {
        xiao_console_print(ui->surface.env, "uitest: failed to init TTF font\r\n");
        return -1;
    }

    ui->font.title_scale = stbtt_ScaleForPixelHeight(&ui->font.font, (float)ui->doc.style.title_px);
    ui->font.body_scale = stbtt_ScaleForPixelHeight(&ui->font.font, (float)ui->doc.style.body_px);

    stbtt_GetFontVMetrics(&ui->font.font, &ascent, &descent, &line_gap);
    ui->font.title_ascent = (int)((float)ascent * ui->font.title_scale + 0.5f);
    ui->font.body_ascent = (int)((float)ascent * ui->font.body_scale + 0.5f);
    ui->font.title_line_h = (int)(((float)(ascent - descent + line_gap)) * ui->font.title_scale + 0.5f);
    ui->font.body_line_h = (int)(((float)(ascent - descent + line_gap)) * ui->font.body_scale + 0.5f);

    if (ui->font.title_line_h < ui->doc.style.title_px) ui->font.title_line_h = ui->doc.style.title_px;
    if (ui->font.body_line_h < ui->doc.style.body_px) ui->font.body_line_h = ui->doc.style.body_px;
    return 0;
}

static void litehtml_compute_layout(ui_state_t *ui) {
    int margin = ui->doc.style.panel_margin;
    int pad = ui->doc.style.panel_padding;
    int button_min = 108;
    int button_w;

    ui->layout.panel_x = margin;
    ui->layout.panel_y = margin;
    ui->layout.panel_w = ui->surface.width - margin * 2;
    ui->layout.panel_h = ui->surface.height - margin * 2;
    if (ui->layout.panel_w < 72) ui->layout.panel_w = 72;
    if (ui->layout.panel_h < 72) ui->layout.panel_h = 72;

    ui->layout.title_x = ui->layout.panel_x + pad;
    ui->layout.title_y = ui->layout.panel_y + pad;

    ui->layout.paragraph_x = ui->layout.panel_x + pad;
    ui->layout.paragraph_y = ui->layout.title_y + ui->font.title_line_h + 10;

    ui->layout.hint_x = ui->layout.panel_x + pad;
    ui->layout.hint_y = ui->layout.paragraph_y + ui->font.body_line_h + 8;

    button_w = text_width_px(&ui->font, ui->font.body_scale, ui->doc.button) + 30;
    if (button_w < button_min) button_w = button_min;
    if (button_w > ui->layout.panel_w - pad * 2) button_w = ui->layout.panel_w - pad * 2;

    ui->layout.button_w = button_w;
    ui->layout.button_h = ui->doc.style.button_height;
    ui->layout.button_x = ui->layout.panel_x + (ui->layout.panel_w - button_w) / 2;
    ui->layout.button_y = ui->layout.hint_y + ui->font.body_line_h + 10;

    ui->layout.button_text_x = ui->layout.button_x + (ui->layout.button_w - text_width_px(&ui->font, ui->font.body_scale, ui->doc.button)) / 2;
    ui->layout.button_text_y = ui->layout.button_y + (ui->layout.button_h - ui->font.body_line_h) / 2;

    ui->layout.status_x = ui->layout.panel_x + pad;
    ui->layout.status_y = ui->layout.button_y + ui->layout.button_h + 12;
    ui->layout.line_step = ui->font.body_line_h + 4;
}

static unsigned int ui_button_fill(const ui_state_t *ui) {
    if (ui->pressing) return ui->doc.style.button_pressed_color;
    if (ui->hovered) return ui->doc.style.button_hover_color;
    return ui->doc.style.button_color;
}

static const char *ui_status_text(const ui_state_t *ui) {
    if (ui->pressing) return "status: pressed";
    if (ui->hovered) return "status: hover";
    if (ui->toggled) return "status: clicked";
    return "status: ready";
}

static void render_root_bg(ui_state_t *ui, int layer_id) {
    (void)layer_id;
    hagl_fill(&ui->surface, ui->doc.style.bg_color);
}

static void render_panel_bg(ui_state_t *ui, int layer_id) {
    ui_layer_t *l = &ui->layers[layer_id];
    hagl_fill_rect(&ui->surface, l->x, l->y, l->w, l->h, ui->doc.style.panel_color);
}

static void render_panel_accent(ui_state_t *ui, int layer_id) {
    ui_layer_t *l = &ui->layers[layer_id];
    hagl_fill_rect(&ui->surface, l->x, l->y, l->w, l->h, ui->doc.style.accent_color);
}

static void render_title(ui_state_t *ui, int layer_id) {
    ui_layer_t *l = &ui->layers[layer_id];
    hagl_fill_rect(&ui->surface, l->x, l->y, l->w, l->h, ui->doc.style.panel_color);
    draw_text(ui, ui->layout.title_x, ui->layout.title_y, ui->doc.title, ui->font.title_scale, ui->font.title_ascent, ui->doc.style.text_color, ui->doc.style.panel_color);
}

static void render_paragraph(ui_state_t *ui, int layer_id) {
    ui_layer_t *l = &ui->layers[layer_id];
    hagl_fill_rect(&ui->surface, l->x, l->y, l->w, l->h, ui->doc.style.panel_color);
    draw_text(ui, ui->layout.paragraph_x, ui->layout.paragraph_y, ui->doc.paragraph, ui->font.body_scale, ui->font.body_ascent, ui->doc.style.text_color, ui->doc.style.panel_color);
}

static void render_hint(ui_state_t *ui, int layer_id) {
    const char *hint = ui->pointer_supported ? "mouse hover/click supported, q or esc exits" : "pointer unsupported, enter/space toggles";
    ui_layer_t *l = &ui->layers[layer_id];
    hagl_fill_rect(&ui->surface, l->x, l->y, l->w, l->h, ui->doc.style.panel_color);
    draw_text(ui, ui->layout.hint_x, ui->layout.hint_y, hint, ui->font.body_scale, ui->font.body_ascent, ui->doc.style.muted_text_color, ui->doc.style.panel_color);
}

static void render_button_bg(ui_state_t *ui, int layer_id) {
    ui_layer_t *l = &ui->layers[layer_id];
    hagl_fill_rect(&ui->surface, l->x, l->y, l->w, l->h, ui_button_fill(ui));
}

static void render_button_text(ui_state_t *ui, int layer_id) {
    ui_layer_t *l = &ui->layers[layer_id];
    unsigned int fill = ui_button_fill(ui);
    hagl_fill_rect(&ui->surface, l->x, l->y, l->w, l->h, fill);
    draw_text(ui, ui->layout.button_text_x, ui->layout.button_text_y, ui->doc.button, ui->font.body_scale, ui->font.body_ascent, ui->doc.style.button_text_color, fill);
}

static void render_status(ui_state_t *ui, int layer_id) {
    ui_layer_t *l = &ui->layers[layer_id];
    hagl_fill_rect(&ui->surface, l->x, l->y, l->w, l->h, ui->doc.style.panel_color);
    draw_text(ui, ui->layout.status_x, ui->layout.status_y, ui_status_text(ui), ui->font.body_scale, ui->font.body_ascent, ui->doc.style.muted_text_color, ui->doc.style.panel_color);
}

static void render_cursor(ui_state_t *ui, int layer_id) {
    int cx;
    int cy;
    (void)layer_id;

    if (!ui->pointer_supported || !ui->pointer_inited) return;

    cx = ui->pointer_x;
    cy = ui->pointer_y;
    hagl_fill_rect(&ui->surface, cx - 1, cy - 1, 3, 3, 0x111827u);
    hagl_fill_rect(&ui->surface, cx - 3, cy, UI_CURSOR_SIZE, 1, 0xFFFFFFu);
    hagl_fill_rect(&ui->surface, cx, cy - 3, 1, UI_CURSOR_SIZE, 0xFFFFFFu);
}

static void layer_set(ui_state_t *ui, int id, int x, int y, int w, int h, layer_render_fn_t render) {
    ui->layers[id].x = x;
    ui->layers[id].y = y;
    ui->layers[id].w = w;
    ui->layers[id].h = h;
    ui->layers[id].render = render;
    ui->layers[id].dirty = 1;
}

static int rect_overlap(int ax, int ay, int aw, int ah, int bx, int by, int bw, int bh) {
    if (aw <= 0 || ah <= 0 || bw <= 0 || bh <= 0) return 0;
    if (ax >= bx + bw) return 0;
    if (bx >= ax + aw) return 0;
    if (ay >= by + bh) return 0;
    if (by >= ay + ah) return 0;
    return 1;
}

static void ui_mark_dirty(ui_state_t *ui, int layer_id) {
    if (layer_id < 0 || layer_id >= UI_LAYER_COUNT) return;
    ui->layers[layer_id].dirty = 1;
}

static void ui_mark_under_rect(ui_state_t *ui, int x, int y, int w, int h) {
    int i;
    for (i = 0; i < UI_LAYER_COUNT; i++) {
        if (i == LAYER_CURSOR) continue;
        if (rect_overlap(x, y, w, h, ui->layers[i].x, ui->layers[i].y, ui->layers[i].w, ui->layers[i].h)) {
            ui->layers[i].dirty = 1;
        }
    }
}

static void ui_mark_cursor_move(ui_state_t *ui, int old_x, int old_y, int new_x, int new_y) {
    int half = UI_CURSOR_SIZE / 2;
    ui_mark_under_rect(ui, old_x - half, old_y - half, UI_CURSOR_SIZE, UI_CURSOR_SIZE);
    ui_mark_under_rect(ui, new_x - half, new_y - half, UI_CURSOR_SIZE, UI_CURSOR_SIZE);
    ui_mark_dirty(ui, LAYER_CURSOR);
}

static void ui_redraw_dirty(ui_state_t *ui) {
    int i;
    for (i = 0; i < UI_LAYER_COUNT; i++) {
        if (!ui->layers[i].dirty || !ui->layers[i].render) continue;
        ui->layers[i].render(ui, i);
        ui->layers[i].dirty = 0;
    }
}

static void ui_mark_interaction_layers(ui_state_t *ui) {
    ui_mark_dirty(ui, LAYER_BUTTON_BG);
    ui_mark_dirty(ui, LAYER_BUTTON_TEXT);
    ui_mark_dirty(ui, LAYER_STATUS_TEXT);
}

static int ui_point_in_button(const ui_state_t *ui, int x, int y) {
    return x >= ui->layout.button_x && x < ui->layout.button_x + ui->layout.button_w &&
           y >= ui->layout.button_y && y < ui->layout.button_y + ui->layout.button_h;
}

static void ui_setup_layers(ui_state_t *ui) {
    int pad = ui->doc.style.panel_padding;

    layer_set(ui, LAYER_ROOT_BG, 0, 0, ui->surface.width, ui->surface.height, render_root_bg);
    layer_set(ui, LAYER_PANEL_BG, ui->layout.panel_x, ui->layout.panel_y, ui->layout.panel_w, ui->layout.panel_h, render_panel_bg);
    layer_set(ui, LAYER_PANEL_ACCENT, ui->layout.panel_x, ui->layout.panel_y, ui->layout.panel_w, 4, render_panel_accent);

    layer_set(ui, LAYER_TITLE_TEXT,
        ui->layout.panel_x + pad,
        ui->layout.title_y,
        ui->layout.panel_w - pad * 2,
        ui->font.title_line_h,
        render_title);

    layer_set(ui, LAYER_PARAGRAPH_TEXT,
        ui->layout.panel_x + pad,
        ui->layout.paragraph_y,
        ui->layout.panel_w - pad * 2,
        ui->font.body_line_h,
        render_paragraph);

    layer_set(ui, LAYER_HINT_TEXT,
        ui->layout.panel_x + pad,
        ui->layout.hint_y,
        ui->layout.panel_w - pad * 2,
        ui->font.body_line_h,
        render_hint);

    layer_set(ui, LAYER_BUTTON_BG,
        ui->layout.button_x,
        ui->layout.button_y,
        ui->layout.button_w,
        ui->layout.button_h,
        render_button_bg);

    layer_set(ui, LAYER_BUTTON_TEXT,
        ui->layout.button_x,
        ui->layout.button_y,
        ui->layout.button_w,
        ui->layout.button_h,
        render_button_text);

    layer_set(ui, LAYER_STATUS_TEXT,
        ui->layout.panel_x + pad,
        ui->layout.status_y,
        ui->layout.panel_w - pad * 2,
        ui->font.body_line_h + 4,
        render_status);

    layer_set(ui, LAYER_CURSOR, 0, 0, ui->surface.width, ui->surface.height, render_cursor);
}

static int ui_poll_pointer(ui_state_t *ui) {
    int x = ui->pointer_x;
    int y = ui->pointer_y;
    int buttons = ui->pointer_buttons;
    int rc = xiao_pointer_read(ui->surface.env, &x, &y, &buttons);
    int changed = 0;

    if (rc < 0) {
        return 0;
    }

    if (!ui->pointer_supported) {
        ui->pointer_supported = 1;
        ui_mark_dirty(ui, LAYER_HINT_TEXT);
        changed = 1;
    }

    if (!ui->pointer_inited) {
        ui->pointer_inited = 1;
        ui->pointer_x = x;
        ui->pointer_y = y;
        ui->pointer_buttons = buttons;
        ui->pointer_prev_x = x;
        ui->pointer_prev_y = y;
        ui->pointer_prev_buttons = buttons;
        ui_mark_dirty(ui, LAYER_CURSOR);
        changed = 1;
    }

    if (x != ui->pointer_x || y != ui->pointer_y) {
        ui_mark_cursor_move(ui, ui->pointer_x, ui->pointer_y, x, y);
        ui->pointer_prev_x = ui->pointer_x;
        ui->pointer_prev_y = ui->pointer_y;
        ui->pointer_x = x;
        ui->pointer_y = y;
        changed = 1;
    }

    if (buttons != ui->pointer_buttons) {
        ui->pointer_prev_buttons = ui->pointer_buttons;
        ui->pointer_buttons = buttons;
        changed = 1;
    }

    {
        int was_hovered = ui->hovered;
        int was_pressing = ui->pressing;
        int was_toggled = ui->toggled;
        int left_now = (ui->pointer_buttons & 1) != 0;
        int left_prev = (ui->pointer_prev_buttons & 1) != 0;

        ui->hovered = ui_point_in_button(ui, ui->pointer_x, ui->pointer_y);
        ui->pressing = ui->hovered && left_now;

        if (left_prev && !left_now && ui->hovered) {
            ui->toggled = !ui->toggled;
        }

        if (was_hovered != ui->hovered || was_pressing != ui->pressing || was_toggled != ui->toggled) {
            ui_mark_interaction_layers(ui);
            changed = 1;
        }
    }

    ui->pointer_prev_buttons = ui->pointer_buttons;
    return changed;
}

int xiao_app_entry(xiao_env *env) {
    static const char demo_html[] =
        "<html><head><style>"
        ":root {"
        "--bg:#E2E8F0;"
        "--panel:#FFFFFF;"
        "--text:#0F172A;"
        "--muted:#475569;"
        "--accent:#2563EB;"
        "--button:#2563EB;"
        "--button-hover:#1D4ED8;"
        "--button-pressed:#0F766E;"
        "--button-text:#F8FAFC;"
        "--panel-margin:10px;"
        "--panel-padding:12px;"
        "--title-px:26px;"
        "--body-px:17px;"
        "--button-height:34px;"
        "}"
        "</style></head><body>"
        "<h1>HAGL + LiteHTML UI Test</h1>"
        "<p>LiteHTML computes layout only. HAGL draws layered rectangles.</p>"
        "<button>Toggle state</button>"
        "</body></html>";
    ui_state_t ui;

    if (hagl_init_surface(&ui.surface, env) != 0) {
        xiao_console_print(env, "uitest: video output unavailable\r\n");
        return 1;
    }

    if (xiao_platform(env) != XIAO_PLATFORM_ESP32) {
        xiao_video_set_mode(env, 1280, 720);
        hagl_init_surface(&ui.surface, env);
    }

    ui.hovered = 0;
    ui.pressing = 0;
    ui.toggled = 0;
    ui.pointer_supported = 0;
    ui.pointer_x = 0;
    ui.pointer_y = 0;
    ui.pointer_buttons = 0;
    ui.pointer_prev_x = 0;
    ui.pointer_prev_y = 0;
    ui.pointer_prev_buttons = 0;
    ui.pointer_inited = 0;

    litehtml_parse_demo(demo_html, &ui.doc);
    if (ui.surface.width < 320) {
        if (ui.doc.style.title_px > 20) ui.doc.style.title_px = 20;
        if (ui.doc.style.body_px > 14) ui.doc.style.body_px = 14;
        if (ui.doc.style.button_height > 28) ui.doc.style.button_height = 28;
    }

    if (ui_font_init(&ui) != 0) return 1;

    litehtml_compute_layout(&ui);
    ui_setup_layers(&ui);
    ui_redraw_dirty(&ui);

    xiao_console_print(env, "uitest: mouse hover/click + enter/space fallback, q exits\r\n");

    while (1) {
        int ch;
        int changed = ui_poll_pointer(&ui);

        ch = xiao_input_read(env);
        if (ch >= 0) {
            if (ch == 'q' || ch == 'Q' || ch == 27) break;

            if (ch == 'h' || ch == 'H') {
                ui.hovered = !ui.hovered;
                ui_mark_interaction_layers(&ui);
                changed = 1;
            } else if (ch == '\r' || ch == '\n' || ch == ' ') {
                ui.toggled = !ui.toggled;
                ui.pressing = 0;
                ui_mark_interaction_layers(&ui);
                changed = 1;
            }
        }

        if (changed) {
            ui_redraw_dirty(&ui);
        } else {
            xiao_wait(env, 10);
        }
    }

    xiao_console_print(env, "uitest: done\r\n");
    return 0;
}

#else

int xiao_app_entry(xiao_env *env) {
    xiao_console_print(env, "uitest: requires TTF build (XIAO_TTF_TERMINAL_DISABLED is set)\r\n");
    return 1;
}

#endif
