#include "xiao.h"
#include <stddef.h>

#if defined(ARDUINO) && defined(ESP32)
extern "C" void esp_restart(void);
#endif

#define TERM_LINE_COUNT 32
#define TERM_LINE_MAX 200
#define TERM_INPUT_MAX 127
#define TERM_GLYPH_BITMAP_MAX (128 * 128)
#define TERM_FONT_PIXELS 24.0f
#define TERM_MARGIN 12
#define TERM_BG_COLOR 0x101318u
#define TERM_TEXT_COLOR 0xE8EEF6u
#define TERM_PROMPT_COLOR 0x8FA4BAu
#define TERM_INPUT_COLOR 0xFFCB52u
#define TERM_CURSOR_COLOR 0xE8EEF6u
#define TERM_PROMPT_STR "> "
#define TTF_ARENA_SIZE (128 * 1024)
#define TERM_FP_SHIFT 16
#define TERM_FP_ONE (1 << TERM_FP_SHIFT)

enum {
    TERM_ACTION_NONE = 0,
    TERM_ACTION_MODE_SWITCH = 1,
    TERM_ACTION_REBOOT = 2,
    TERM_ACTION_SHUTDOWN = 3
};

enum {
    TERM_ROW_KIND_NONE = 0,
    TERM_ROW_KIND_HISTORY = 1,
    TERM_ROW_KIND_INPUT = 2
};

typedef struct {
    unsigned int fg;
    unsigned int table[256];
} term_color_lut;

static int streq(const char *a, const char *b) {
    while (*a && *b && *a == *b) {
        a++;
        b++;
    }
    return *a == 0 && *b == 0;
}

static xiao_size xstrlen(const char *s) {
    xiao_size n = 0;
    while (s && s[n]) n++;
    return n;
}

static int parse_mode_name(const char *s) {
    if (!s) return -1;
    if (streq(s, "text")) return XIAO_MODE_TEXT;
    if (streq(s, "cli")) return XIAO_MODE_CLI;
    if (streq(s, "gui")) return XIAO_MODE_GUI;
    return -1;
}

static int token_streq(const char *line, const char *word) {
    xiao_size i = 0;
    if (!line || !word) return 0;
    while (word[i]) {
        if (line[i] != word[i]) return 0;
        i++;
    }
    return line[i] == 0 || line[i] == ' ' || line[i] == '\t';
}

static int terminal_action_for_line(const char *line) {
    if (token_streq(line, "reboot")) return TERM_ACTION_REBOOT;
    if (token_streq(line, "shutdown")) return TERM_ACTION_SHUTDOWN;
    if (token_streq(line, "poweroff")) return TERM_ACTION_SHUTDOWN;
    if (token_streq(line, "halt")) return TERM_ACTION_SHUTDOWN;
    return TERM_ACTION_NONE;
}

int xiao_uefi_reboot(void) __attribute__((weak));
int xiao_uefi_shutdown(void) __attribute__((weak));

static void do_reboot(xiao_env *env) {
#if defined(ARDUINO) && defined(ESP32)
    if (xiao_platform(env) == XIAO_PLATFORM_ESP32) {
        esp_restart();
        while (1) xiao_wait(env, 1000);
    }
#endif
    if (xiao_uefi_reboot && xiao_uefi_reboot() == 0) {
        while (1) xiao_wait(env, 1000);
    }
}

static void do_shutdown(xiao_env *env) {
    if (xiao_uefi_shutdown && xiao_uefi_shutdown() == 0) {
        while (1) xiao_wait(env, 1000);
    }
}

static int run_text_terminal(xiao_env *env);

#ifndef XIAO_TTF_TERMINAL_DISABLED
#define STBTT_STATIC
#ifndef NULL
#define NULL ((void *)0)
#endif

typedef struct {
    unsigned int used;
    unsigned char buf[TTF_ARENA_SIZE];
} ttf_arena_state;

static ttf_arena_state ttf_arena;

static void *ttf_malloc(unsigned int size, void *userdata) {
    unsigned int aligned;
    (void)userdata;
    if (size == 0) size = 1;
    aligned = (size + 7u) & ~7u;
    if (ttf_arena.used + aligned > TTF_ARENA_SIZE) return 0;
    {
        void *ptr = ttf_arena.buf + ttf_arena.used;
        ttf_arena.used += aligned;
        return ptr;
    }
}

static void ttf_free(void *ptr, void *userdata) {
    (void)ptr;
    (void)userdata;
}

static void ttf_reset_alloc(void) {
    ttf_arena.used = 0;
}

static void *ttf_memcpy(void *dst, const void *src, xiao_size n) {
    xiao_size i;
    unsigned char *d = (unsigned char *)dst;
    const unsigned char *s = (const unsigned char *)src;
    for (i = 0; i < n; i++) d[i] = s[i];
    return dst;
}

static void *ttf_memset(void *dst, int value, xiao_size n) {
    xiao_size i;
    unsigned char *d = (unsigned char *)dst;
    for (i = 0; i < n; i++) d[i] = (unsigned char)value;
    return dst;
}

static float ttf_fabs(float x) {
    return x < 0.0f ? -x : x;
}

static int ttf_ifloor(float x) {
    int i = (int)x;
    return (i > x) ? (i - 1) : i;
}

static int ttf_iceil(float x) {
    int i = (int)x;
    return (i < x) ? (i + 1) : i;
}

static float ttf_sqrt(float x) {
    float r;
    int i;
    if (x <= 0.0f) return 0.0f;
    r = x > 1.0f ? x : 1.0f;
    for (i = 0; i < 8; i++) {
        r = 0.5f * (r + x / r);
    }
    return r;
}

static float ttf_fmod(float x, float y) {
    int q;
    if (y == 0.0f) return 0.0f;
    q = (int)(x / y);
    return x - (float)q * y;
}

static float ttf_pow(float x, float y) {
    int e;
    float r = 1.0f;
    if (y == 0.0f) return 1.0f;
    e = (int)y;
    if ((float)e != y || e < 0) {
        return x > 0.0f ? ttf_sqrt(x) : 0.0f;
    }
    while (e > 0) {
        r *= x;
        e--;
    }
    return r;
}

static float ttf_cos(float x) {
    float x2;
    while (x > 3.1415926f) x -= 6.2831852f;
    while (x < -3.1415926f) x += 6.2831852f;
    x2 = x * x;
    return 1.0f - x2 * 0.5f + (x2 * x2) * (1.0f / 24.0f);
}

static float ttf_acos(float x) {
    float y;
    if (x <= -1.0f) return 3.1415926f;
    if (x >= 1.0f) return 0.0f;
    y = ttf_sqrt(1.0f - x * x);
    if (x == 0.0f) return 1.5707963f;
    if (x > 0.0f) return y;
    return 3.1415926f - y;
}

#define STBTT_assert(x) ((void)(x))
#define STBTT_malloc(x,u) ttf_malloc((unsigned int)(x), (u))
#define STBTT_free(x,u) ttf_free((x), (u))
#define STBTT_strlen(x) xstrlen((x))
#define STBTT_memcpy(d,s,n) ttf_memcpy((d), (s), (n))
#define STBTT_memset(d,v,n) ttf_memset((d), (v), (n))
#define STBTT_ifloor(x) ttf_ifloor((float)(x))
#define STBTT_iceil(x) ttf_iceil((float)(x))
#define STBTT_sqrt(x) ttf_sqrt((float)(x))
#define STBTT_pow(x,y) ttf_pow((float)(x), (float)(y))
#define STBTT_fmod(x,y) ttf_fmod((float)(x), (float)(y))
#define STBTT_cos(x) ttf_cos((float)(x))
#define STBTT_acos(x) ttf_acos((float)(x))
#define STBTT_fabs(x) ttf_fabs((float)(x))
#define STB_TRUETYPE_IMPLEMENTATION
#include "stb_truetype.h"

typedef struct {
    xiao_env *env;
    stbtt_fontinfo font;
    float scale;
    int scale_fp;
    int ascent_px;
    int line_height;
    int screen_w;
    int screen_h;
    int esc_state;
    int csi_len;
    char csi_buf[16];
    int line_count;
    int line_head;
    int line_len[TERM_LINE_COUNT];
    char lines[TERM_LINE_COUNT][TERM_LINE_MAX];
    char input[TERM_INPUT_MAX + 1];
    int input_len;
    int output_rows;
    int view_start;
    int output_dirty_all;
    int input_dirty;
    int cursor_prev_x;
    int input_row_init;
    int input_row;
    int prev_input_len;
    char prev_input[TERM_INPUT_MAX + 1];
    int row_cache_len[TERM_LINE_COUNT];
    unsigned char row_cache_kind[TERM_LINE_COUNT];
    char row_cache_text[TERM_LINE_COUNT][TERM_LINE_MAX];
    term_color_lut lut_text;
    term_color_lut lut_prompt;
    term_color_lut lut_input;
    unsigned char dirty_rows[TERM_LINE_COUNT];
} cli_term_state;

static cli_term_state cli_state;

static int term_max(int a, int b) {
    return a > b ? a : b;
}

static int term_line_slot(cli_term_state *st, int logical_index) {
    if (logical_index < 0) return 0;
    return (st->line_head + logical_index) % TERM_LINE_COUNT;
}

static void term_copy_cstr_cap(char *dst, int cap, const char *src, int *out_len) {
    int i = 0;
    if (cap <= 0) return;
    while (src && src[i] && i + 1 < cap) {
        dst[i] = src[i];
        i++;
    }
    dst[i] = 0;
    if (out_len) *out_len = i;
}

static int term_cstr_eq_n(const char *a, int a_len, const char *b, int b_len) {
    int i;
    if (a_len != b_len) return 0;
    for (i = 0; i < a_len; i++) {
        if (a[i] != b[i]) return 0;
    }
    return 1;
}

static void term_init_color_lut(term_color_lut *lut, unsigned int fg) {
    int a;
    unsigned int bg_r = (TERM_BG_COLOR >> 16) & 0xffu;
    unsigned int bg_g = (TERM_BG_COLOR >> 8) & 0xffu;
    unsigned int bg_b = TERM_BG_COLOR & 0xffu;
    unsigned int fg_r = (fg >> 16) & 0xffu;
    unsigned int fg_g = (fg >> 8) & 0xffu;
    unsigned int fg_b = fg & 0xffu;
    lut->fg = fg;
    for (a = 0; a < 256; a++) {
        unsigned int ia = (unsigned int)(255 - a);
        unsigned int r = (fg_r * (unsigned int)a + bg_r * ia) / 255u;
        unsigned int g = (fg_g * (unsigned int)a + bg_g * ia) / 255u;
        unsigned int b = (fg_b * (unsigned int)a + bg_b * ia) / 255u;
        lut->table[a] = (r << 16) | (g << 8) | b;
    }
}

static const term_color_lut *term_pick_lut(cli_term_state *st, unsigned int color) {
    if (color == st->lut_text.fg) return &st->lut_text;
    if (color == st->lut_prompt.fg) return &st->lut_prompt;
    if (color == st->lut_input.fg) return &st->lut_input;
    return 0;
}

static void cli_mark_all_output_dirty(cli_term_state *st) {
    int i;
    for (i = 0; i < TERM_LINE_COUNT; i++) st->dirty_rows[i] = 0;
    for (i = 0; i < st->output_rows; i++) st->dirty_rows[i] = 1;
    st->output_dirty_all = 1;
    st->input_row_init = 0;
    st->input_row = -1;
    for (i = 0; i < TERM_LINE_COUNT; i++) st->row_cache_kind[i] = TERM_ROW_KIND_NONE;
}

static void cli_mark_output_row_dirty(cli_term_state *st, int row) {
    if (row < 0 || row >= st->output_rows || row >= TERM_LINE_COUNT) return;
    st->dirty_rows[row] = 1;
}

static void cli_mark_history_line_dirty(cli_term_state *st, int history_index) {
    int row = history_index - st->view_start;
    cli_mark_output_row_dirty(st, row);
}

static int cli_input_logical_index(cli_term_state *st) {
    int last;
    int slot;
    if (!st || st->line_count <= 0) return 0;
    last = st->line_count - 1;
    slot = term_line_slot(st, last);
    if (st->line_len[slot] == 0) return last;
    return st->line_count;
}

static void cli_mark_input_line_dirty(cli_term_state *st) {
    int row = cli_input_logical_index(st) - st->view_start;
    cli_mark_output_row_dirty(st, row);
}

static int cli_sync_view(cli_term_state *st) {
    int old = st->view_start;
    int total_lines = cli_input_logical_index(st) + 1;
    int max_start = total_lines - st->output_rows;
    if (max_start < 0) max_start = 0;
    st->view_start = max_start;
    if (st->view_start != old) {
        cli_mark_all_output_dirty(st);
        return 1;
    }
    return 0;
}

static void cli_clear_history(cli_term_state *st) {
    int i;
    st->line_count = 1;
    st->line_head = 0;
    st->esc_state = 0;
    st->view_start = 0;
    for (i = 0; i < TERM_LINE_COUNT; i++) {
        st->line_len[i] = 0;
        st->lines[i][0] = 0;
    }
    cli_mark_all_output_dirty(st);
    st->input_dirty = 0;
    st->input_row_init = 0;
}

static void cli_new_line(cli_term_state *st) {
    int view_changed = 0;

    if (st->line_count < TERM_LINE_COUNT) {
        int slot = term_line_slot(st, st->line_count);
        st->line_len[slot] = 0;
        st->lines[slot][0] = 0;
        st->line_count++;
        view_changed = cli_sync_view(st);
        if (!view_changed) {
            cli_mark_history_line_dirty(st, st->line_count - 2);
            cli_mark_history_line_dirty(st, st->line_count - 1);
            cli_mark_input_line_dirty(st);
        }
    } else {
        st->line_head = (st->line_head + 1) % TERM_LINE_COUNT;
        {
            int slot = term_line_slot(st, st->line_count - 1);
            st->line_len[slot] = 0;
            st->lines[slot][0] = 0;
        }
        cli_sync_view(st);
        cli_mark_all_output_dirty(st);
    }
}

static void cli_push_char(cli_term_state *st, char c) {
    int logical_idx = st->line_count - 1;
    int idx = term_line_slot(st, logical_idx);
    int len = st->line_len[idx];
    if (c == '\t') {
        int k;
        for (k = 0; k < 4; k++) cli_push_char(st, ' ');
        return;
    }
    if (len + 1 >= TERM_LINE_MAX) {
        cli_new_line(st);
        logical_idx = st->line_count - 1;
        idx = term_line_slot(st, logical_idx);
        len = st->line_len[idx];
    }
    st->lines[idx][len] = c;
    st->lines[idx][len + 1] = 0;
    st->line_len[idx] = len + 1;
    cli_mark_history_line_dirty(st, logical_idx);
}

static void cli_feed_output(cli_term_state *st, const char *data, xiao_size len) {
    xiao_size i;

    for (i = 0; i < len; i++) {
        unsigned char c = (unsigned char)data[i];
        if (st->esc_state == 1) {
            if (c == '[') {
                st->esc_state = 2;
                st->csi_len = 0;
                st->csi_buf[0] = 0;
            } else {
                st->esc_state = 0;
            }
            continue;
        }
        if (st->esc_state == 2) {
            if (c >= 0x40 && c <= 0x7e) {
                int p0 = 0;
                int has_digit = 0;
                int k = 0;
                st->csi_buf[st->csi_len] = 0;
                while (st->csi_buf[k] && st->csi_buf[k] != ';') {
                    if (st->csi_buf[k] >= '0' && st->csi_buf[k] <= '9') {
                        has_digit = 1;
                        p0 = p0 * 10 + (st->csi_buf[k] - '0');
                    } else {
                        has_digit = 0;
                        break;
                    }
                    k++;
                }
                if (c == 'J') {
                    if (!has_digit || p0 == 2 || p0 == 3) {
                        cli_clear_history(st);
                    }
                } else if (c == 'H' || c == 'f') {
                    cli_mark_all_output_dirty(st);
                }
                st->esc_state = 0;
                st->csi_len = 0;
                continue;
            }
            if ((c >= '0' && c <= '9') || c == ';' || c == '?') {
                if (st->csi_len + 1 < (int)sizeof(st->csi_buf)) {
                    st->csi_buf[st->csi_len++] = (char)c;
                }
                continue;
            }
            st->esc_state = 0;
            st->csi_len = 0;
            continue;
        }

        if (c == 0x1b) {
            st->esc_state = 1;
            continue;
        }
        if (c == '\r') continue;
        if (c == '\n') {
            cli_new_line(st);
            continue;
        }
        if (c == 0x08 || c == 0x7f) {
            int logical_idx = st->line_count - 1;
            int idx = term_line_slot(st, logical_idx);
            if (st->line_len[idx] > 0) {
                st->line_len[idx]--;
                st->lines[idx][st->line_len[idx]] = 0;
                cli_mark_history_line_dirty(st, logical_idx);
            }
            continue;
        }
        if (c >= 32 && c <= 126) {
            cli_push_char(st, (char)c);
        }
    }
}

static int term_advance_px(cli_term_state *st, int cp, int next_cp) {
    int adv = 0;
    int lsb = 0;
    int px_fp;
    stbtt_GetCodepointHMetrics(&st->font, cp, &adv, &lsb);
    (void)lsb;
    px_fp = adv * st->scale_fp;
    if (next_cp > 0) {
        px_fp += stbtt_GetCodepointKernAdvance(&st->font, cp, next_cp) * st->scale_fp;
    }
    if (px_fp >= 0) return (px_fp + (TERM_FP_ONE >> 1)) >> TERM_FP_SHIFT;
    return -(((-px_fp) + (TERM_FP_ONE >> 1)) >> TERM_FP_SHIFT);
}

static int term_blend_rgb888(unsigned int bg, unsigned int fg, unsigned char a) {
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

static int draw_glyph(cli_term_state *st, int pen_x, int baseline_y, int cp, int next_cp, unsigned int color) {
    static unsigned char bitmap[TERM_GLYPH_BITMAP_MAX];
    static unsigned int rgbbuf[TERM_GLYPH_BITMAP_MAX];
    const term_color_lut *lut = term_pick_lut(st, color);
    int x0, y0, x1, y1;
    int w, h;
    int gx, gy;

    if (cp < 32 || cp > 126) cp = '?';

    stbtt_GetCodepointBitmapBoxSubpixel(&st->font, cp, st->scale, st->scale, 0.0f, 0.0f, &x0, &y0, &x1, &y1);
    w = x1 - x0;
    h = y1 - y0;

    if (w > 0 && h > 0 && w * h <= TERM_GLYPH_BITMAP_MAX) {
        ttf_reset_alloc();
        stbtt_MakeCodepointBitmapSubpixel(&st->font, bitmap, w, h, w, st->scale, st->scale, 0.0f, 0.0f, cp);

        for (gy = 0; gy < h; gy++) {
            for (gx = 0; gx < w; gx++) {
                unsigned char a = bitmap[gy * w + gx];
                if (lut) {
                    rgbbuf[gy * w + gx] = lut->table[a];
                } else {
                    rgbbuf[gy * w + gx] = term_blend_rgb888(TERM_BG_COLOR, color, a);
                }
            }
        }

        xiao_video_blit_rgb888(st->env, pen_x + x0, baseline_y + y0, w, h, rgbbuf, w);
    }

    pen_x += term_advance_px(st, cp, next_cp);
    return pen_x;
}
static int text_width_n(cli_term_state *st, const char *text, int n) {
    int width = 0;
    int i = 0;
    while (text[i] && (n < 0 || i < n)) {
        int cp = (unsigned char)text[i];
        int next_cp = text[i + 1] ? (unsigned char)text[i + 1] : 0;
        width += term_advance_px(st, cp, next_cp);
        i++;
    }
    return width;
}

static int text_width(cli_term_state *st, const char *text) {
    return text_width_n(st, text, -1);
}

static int common_prefix_len(const char *a, const char *b) {
    int n = 0;
    while (a[n] && b[n] && a[n] == b[n]) n++;
    return n;
}

static void draw_text_from_index(cli_term_state *st, int x, int y, const char *text, int start, unsigned int color) {
    int pen_x = x;
    int baseline = y + st->ascent_px;
    int text_len = 0;
    int i;
    int cpv[TERM_LINE_MAX];
    int adv[TERM_LINE_MAX];

    while (text[text_len] && text_len < TERM_LINE_MAX - 1) {
        cpv[text_len] = (unsigned char)text[text_len];
        text_len++;
    }
    for (i = 0; i < text_len; i++) {
        int next_cp = (i + 1 < text_len) ? cpv[i + 1] : 0;
        adv[i] = term_advance_px(st, cpv[i], next_cp);
    }
    for (i = 0; i < text_len; i++) {
        int next_cp = (i + 1 < text_len) ? cpv[i + 1] : 0;
        if (i >= start) {
            pen_x = draw_glyph(st, pen_x, baseline, cpv[i], next_cp, color);
        } else {
            pen_x += adv[i];
        }
        if (pen_x >= st->screen_w - TERM_MARGIN) break;
    }
}

static void draw_text(cli_term_state *st, int x, int y, const char *text, unsigned int color) {
    draw_text_from_index(st, x, y, text, 0, color);
}

static int output_y_for_row(cli_term_state *st, int row) {
    return TERM_MARGIN + row * st->line_height;
}

static void render_output_row(cli_term_state *st, int row) {
    int y;
    int h;
    int idx;
    int input_idx;
    y = output_y_for_row(st, row);
    h = st->line_height;
    idx = st->view_start + row;
    input_idx = cli_input_logical_index(st);
    if (idx >= 0 && idx < st->line_count && idx != input_idx) {
        int slot = term_line_slot(st, idx);
        int new_len = st->line_len[slot];
        int old_len = st->row_cache_len[row];
        int redraw_from = 0;
        int redraw_x;
        int old_width;
        int new_width;
        int clear_end;

        if (st->row_cache_kind[row] == TERM_ROW_KIND_HISTORY &&
            term_cstr_eq_n(st->row_cache_text[row], old_len, st->lines[slot], new_len)) {
            return;
        }
        if (st->row_cache_kind[row] == TERM_ROW_KIND_HISTORY) {
            int prefix = common_prefix_len(st->row_cache_text[row], st->lines[slot]);
            redraw_from = prefix > 0 ? prefix - 1 : 0;
        }
        redraw_x = TERM_MARGIN + text_width_n(st, st->lines[slot], redraw_from);
        old_width = text_width_n(st, st->row_cache_text[row], old_len);
        new_width = text_width_n(st, st->lines[slot], new_len);
        clear_end = TERM_MARGIN + term_max(old_width, new_width) + 12;
        if (st->row_cache_kind[row] != TERM_ROW_KIND_HISTORY || redraw_x < TERM_MARGIN) {
            redraw_x = 0;
        }
        if (clear_end < redraw_x + 12) clear_end = redraw_x + 12;
        if (clear_end > st->screen_w) clear_end = st->screen_w;
        xiao_video_fill_rect_rgb888(st->env, redraw_x, y, clear_end - redraw_x, h, TERM_BG_COLOR);
        draw_text_from_index(st, TERM_MARGIN, y, st->lines[slot], redraw_from, TERM_TEXT_COLOR);
        st->row_cache_kind[row] = TERM_ROW_KIND_HISTORY;
        term_copy_cstr_cap(st->row_cache_text[row], TERM_LINE_MAX, st->lines[slot], &st->row_cache_len[row]);
        return;
    }
    if (idx == input_idx) {
        int prompt_x = TERM_MARGIN + text_width_n(st, TERM_PROMPT_STR, -1);
        int old_width = text_width_n(st, st->row_cache_text[row], st->row_cache_len[row]);
        int new_width = text_width(st, st->input);
        int prefix = st->row_cache_kind[row] == TERM_ROW_KIND_INPUT ? common_prefix_len(st->row_cache_text[row], st->input) : 0;
        int redraw_from = prefix > 0 ? prefix - 1 : 0;
        int redraw_x = prompt_x + text_width_n(st, st->input, redraw_from);
        int clear_end = prompt_x + (old_width > new_width ? old_width : new_width) + 12;
        int cursor_x;

        if (st->input_row != row) {
            st->input_row = row;
            st->input_row_init = 0;
        }

        if (!st->input_row_init || st->row_cache_kind[row] != TERM_ROW_KIND_INPUT) {
            xiao_video_fill_rect_rgb888(st->env, 0, y, st->screen_w, h, TERM_BG_COLOR);
            draw_text(st, TERM_MARGIN, y, TERM_PROMPT_STR, TERM_PROMPT_COLOR);
            draw_text(st, prompt_x, y, st->input, TERM_INPUT_COLOR);
            st->input_row_init = 1;
        } else {
            if (redraw_x < prompt_x) redraw_x = prompt_x;
            if (clear_end < redraw_x + 12) clear_end = redraw_x + 12;
            if (clear_end > st->screen_w) clear_end = st->screen_w;
            xiao_video_fill_rect_rgb888(st->env, redraw_x, y, clear_end - redraw_x, h, TERM_BG_COLOR);
            draw_text(st, TERM_MARGIN, y, TERM_PROMPT_STR, TERM_PROMPT_COLOR);
            draw_text_from_index(st, prompt_x, y, st->input, redraw_from, TERM_INPUT_COLOR);
        }

        cursor_x = prompt_x + new_width;
        {
            int cursor_y = y + 3;
            int cursor_h = h - 6;
            if (cursor_h < 10) cursor_h = 10;
            if (cursor_y + cursor_h > st->screen_h) cursor_h = st->screen_h - cursor_y;
            if (cursor_h > 0) xiao_video_fill_rect_rgb888(st->env, cursor_x, cursor_y, 2, cursor_h, TERM_CURSOR_COLOR);
        }
        st->cursor_prev_x = cursor_x;
        st->prev_input_len = st->input_len;
        term_copy_cstr_cap(st->prev_input, TERM_INPUT_MAX + 1, st->input, 0);
        st->row_cache_kind[row] = TERM_ROW_KIND_INPUT;
        term_copy_cstr_cap(st->row_cache_text[row], TERM_LINE_MAX, st->input, &st->row_cache_len[row]);
        return;
    }
    if (st->row_cache_kind[row] != TERM_ROW_KIND_NONE) {
        xiao_video_fill_rect_rgb888(st->env, 0, y, st->screen_w, h, TERM_BG_COLOR);
        st->row_cache_kind[row] = TERM_ROW_KIND_NONE;
        st->row_cache_len[row] = 0;
        st->row_cache_text[row][0] = 0;
    }
}

static void cli_render_dirty(cli_term_state *st) {
    int i;
    if (st->input_dirty) {
        cli_mark_input_line_dirty(st);
        st->input_dirty = 0;
    }
    for (i = 0; i < st->output_rows; i++) {
        if (!st->dirty_rows[i]) continue;
        render_output_row(st, i);
        st->dirty_rows[i] = 0;
    }
    st->output_dirty_all = 0;
}

static void cli_init_layers(cli_term_state *st) {
    int i;
    st->output_rows = (st->screen_h - TERM_MARGIN * 2) / st->line_height;
    if (st->output_rows < 1) st->output_rows = 1;
    if (st->output_rows > TERM_LINE_COUNT - 1) st->output_rows = TERM_LINE_COUNT - 1;

    st->view_start = 0;
    st->cursor_prev_x = -1;
    st->output_dirty_all = 0;
    st->input_dirty = 0;
    st->input_row_init = 0;
    st->input_row = -1;
    st->prev_input_len = 0;
    st->prev_input[0] = 0;
    for (i = 0; i < TERM_LINE_COUNT; i++) {
        st->row_cache_kind[i] = TERM_ROW_KIND_NONE;
        st->row_cache_len[i] = 0;
        st->row_cache_text[i][0] = 0;
    }
    term_init_color_lut(&st->lut_text, TERM_TEXT_COLOR);
    term_init_color_lut(&st->lut_prompt, TERM_PROMPT_COLOR);
    term_init_color_lut(&st->lut_input, TERM_INPUT_COLOR);
    xiao_video_fill_rect_rgb888(st->env, 0, 0, st->screen_w, st->screen_h, TERM_BG_COLOR);
    cli_mark_all_output_dirty(st);
}

static int cli_sink(void *ctx, const char *data, xiao_size len) {
    cli_term_state *st = (cli_term_state *)ctx;
    if (!st || !data || len == 0) return 1;
    cli_feed_output(st, data, len);
    cli_render_dirty(st);
    return 1;
}
#endif

static int run_text_terminal(xiao_env *env) {
    char line[128];
    xiao_size n = 0;
    int action = TERM_ACTION_NONE;
    int run_mode = xiao_mode_get();

    xiao_console_print(env, "xiao terminal ready\r\n");
    xiao_console_print(env, "type command (example: ls, cat readme.txt), or 'exit'\r\n");

    while (1) {
        int ch;
        xiao_console_print(env, "> ");
        n = 0;

        while (1) {
            ch = xiao_input_read(env);
            if (ch < 0) {
                xiao_wait(env, 10);
                continue;
            }
            if (ch == '\r' || ch == '\n') {
                xiao_console_print(env, "\r\n");
                break;
            }
            if ((ch == 0x08 || ch == 0x7f) && n > 0) {
                n--;
                xiao_console_print(env, "\b \b");
                continue;
            }
            if (ch >= 32 && ch <= 126 && n + 1 < sizeof(line)) {
                line[n++] = (char)ch;
                {
                    char out[2];
                    out[0] = (char)ch;
                    out[1] = 0;
                    xiao_console_print(env, out);
                }
            }
        }

        line[n] = 0;
        if (n == 0) continue;
        action = terminal_action_for_line(line);
        if (action != TERM_ACTION_NONE) return action;
        if (streq(line, "exit")) break;

        if (xiao_exec_line(line) != 0) {
            xiao_console_print(env, "command failed: ");
            xiao_console_print(env, line);
            xiao_console_print(env, "\r\n");
        }

        if (xiao_mode_get() != run_mode) {
            return TERM_ACTION_MODE_SWITCH;
        }
    }

    xiao_console_print(env, "terminal closed\r\n");
    return TERM_ACTION_NONE;
}

#ifdef XIAO_TTF_TERMINAL_DISABLED
static int run_cli_terminal(xiao_env *env) {
    xiao_console_print(env, "terminal(cli): not available on this build, falling back to text mode\r\n");
    return run_text_terminal(env);
}
#else
#include "gui_proto.h"

// gui_proto.h で extern 宣言されているため、ここでは宣言を削除

static int run_cli_terminal(xiao_env *env) {
    const char *font_data = 0;
    xiao_size font_size = 0;
    int ascent = 0;
    int descent = 0;
    int line_gap = 0;
    int action = TERM_ACTION_NONE;
    int run_mode = xiao_mode_get();
    cli_term_state *st = &cli_state;
    xiao_ipc_message msg;

    if (xiao_platform(env) != XIAO_PLATFORM_ESP32) {
        xiao_video_set_mode(env, 1280, 720);
    }

    if (xiao_video_size(env, &st->screen_w, &st->screen_h) != 0) {
        xiao_console_print(env, "terminal(cli): video is not available\r\n");
        return 1;
    }

    if (xiao_fs_read("/IBMPlexMono-Regular.ttf", &font_data, &font_size) != 0) {
        xiao_console_print(env, "terminal(cli): /IBMPlexMono-Regular.ttf not found\r\n");
        return 1;
    }

    if (font_size < 1024) {
        xiao_console_print(env, "terminal(cli): font data is invalid\r\n");
        return 1;
    }

    st->env = env;
    st->input_len = 0;
    st->input[0] = 0;

    if (!stbtt_InitFont(&st->font, (const unsigned char *)font_data, stbtt_GetFontOffsetForIndex((const unsigned char *)font_data, 0))) {
        xiao_console_print(env, "terminal(cli): failed to init font\r\n");
        return 1;
    }

    st->scale = stbtt_ScaleForPixelHeight(&st->font, TERM_FONT_PIXELS);
    st->scale_fp = (int)(st->scale * (float)TERM_FP_ONE + 0.5f);
    stbtt_GetFontVMetrics(&st->font, &ascent, &descent, &line_gap);
    st->ascent_px = (int)((float)ascent * st->scale + 0.5f);
    st->line_height = (int)(((float)(ascent - descent + line_gap)) * st->scale + 0.5f);
    if (st->line_height < 18) st->line_height = 18;

    cli_init_layers(st);
    cli_clear_history(st);
    cli_feed_output(st, "BaramOS Terminal ready\n", xstrlen("BaramOS Terminal ready\n"));
    cli_feed_output(st, "Hello, world!\n", xstrlen("Hello, world!\n"));

    xiao_console_set_sink(cli_sink, st);
    cli_render_dirty(st);

    while (1) {
        if (xiao_mode_get() == XIAO_MODE_GUI) {
            xiao_ipc_message msg;
            if (xiao_ipc_receive(&msg) == 0 && msg.type == GUI_CMD_FRAME_READY) {
                int sw, sh;
                xiao_video_size(env, &sw, &sh);
                xiao_video_blit_rgb888(env, 0, 0, sw, sh, gui_framebuffer, sw);
            }
        }

        int ch = xiao_input_read(env);
        if (ch >= 0) {
            if (ch == '\r' || ch == '\n') {
                st->input[st->input_len] = 0;
                if (xiao_mode_get() != XIAO_MODE_GUI) {
                    cli_feed_output(st, TERM_PROMPT_STR, xstrlen(TERM_PROMPT_STR));
                    cli_feed_output(st, st->input, xstrlen(st->input));
                    cli_feed_output(st, "\n", 1);
                }

                if (st->input_len > 0) {
                    action = terminal_action_for_line(st->input);
                    if (action != TERM_ACTION_NONE) break;
                    if (streq(st->input, "exit")) break;
                    if (xiao_exec_line(st->input) != 0) {
                        if (xiao_mode_get() != XIAO_MODE_GUI) {
                            cli_feed_output(st, "command failed: ", xstrlen("command failed: "));
                            cli_feed_output(st, st->input, xstrlen(st->input));
                            cli_feed_output(st, "\n", 1);
                        }
                    }
                }

                if (xiao_mode_get() != run_mode) {
                    action = TERM_ACTION_MODE_SWITCH;
                    break;
                }

                st->input_len = 0;
                st->input[0] = 0;
                st->input_dirty = 1;
                if (xiao_mode_get() != XIAO_MODE_GUI) cli_render_dirty(st);
            } else if ((ch == 0x08 || ch == 0x7f) && st->input_len > 0) {
                st->input_len--;
                st->input[st->input_len] = 0;
                st->input_dirty = 1;
                if (xiao_mode_get() != XIAO_MODE_GUI) cli_render_dirty(st);
            } else if (ch >= 32 && ch <= 126 && st->input_len < TERM_INPUT_MAX) {
                st->input[st->input_len++] = (char)ch;
                st->input[st->input_len] = 0;
                st->input_dirty = 1;
                if (xiao_mode_get() != XIAO_MODE_GUI) cli_render_dirty(st);
            }
        }
        xiao_wait(env, 10);
    }

    xiao_console_set_sink(0, 0);
    if (action != TERM_ACTION_NONE) return action;
    cli_feed_output(st, "terminal closed\n", xstrlen("terminal closed\n"));
    st->input_dirty = 1;
    cli_render_dirty(st);
    return TERM_ACTION_NONE;
}
#endif


int xiao_app_entry(xiao_env *env) {
    int mode = xiao_mode_get();
    int warned_gui = 0;

    if (xiao_argc(env) == 3 && streq(xiao_argv(env, 1), "-m")) {
        int parsed = parse_mode_name(xiao_argv(env, 2));
        if (parsed < 0) {
            xiao_console_print(env, "usage: terminal [-m text|cli|gui]\r\n");
            return 1;
        }
        mode = parsed;
        xiao_mode_set(mode);
    } else if (xiao_argc(env) != 1) {
        xiao_console_print(env, "usage: terminal [-m text|cli|gui]\r\n");
        return 1;
    }

    while (1) {
        int rc;
        mode = xiao_mode_get();
        if (mode == XIAO_MODE_TEXT) {
            rc = run_text_terminal(env);
        } else {
            rc = run_cli_terminal(env);
        }

        if (rc == TERM_ACTION_MODE_SWITCH) {
            if (xiao_mode_get() == XIAO_MODE_TEXT) {
                xiao_console_print(env, "\x1b[2J\x1b[H");
            } else {
                xiao_video_fill_rgb888(env, TERM_BG_COLOR);
            }
            continue;
        }

        if (rc == TERM_ACTION_REBOOT) {
            do_reboot(env);
            xiao_fs_chdir("/");
            if (mode == XIAO_MODE_TEXT) {
                xiao_console_print(env, "\x1b[2J\x1b[H");
            } else {
                xiao_video_fill_rgb888(env, TERM_BG_COLOR);
            }
            continue;
        }

        if (rc == TERM_ACTION_SHUTDOWN) {
            do_shutdown(env);
            xiao_fs_chdir("/");
            if (mode == XIAO_MODE_TEXT) {
                xiao_console_print(env, "\x1b[2J\x1b[H");
            } else {
                xiao_video_fill_rgb888(env, TERM_BG_COLOR);
            }
            xiao_console_print(env, "system halted\r\n");
            while (1) xiao_wait(env, 1000);
        }

        return rc;
    }
}
