#include <cstring>
#include <cctype>
#include <string>

#include "litehtml.h"

extern "C" {

typedef struct {
    int ok;
    int title_x;
    int title_y;
    int title_w;
    int title_h;
    int paragraph_x;
    int paragraph_y;
    int paragraph_w;
    int paragraph_h;
    int button_x;
    int button_y;
    int button_w;
    int button_h;
    char title[96];
    char paragraph[160];
    char button[64];
} xiao_litehtml_layout_result;

}

namespace {

struct font_handle {
    float px;
};

class xiao_doc_container final : public litehtml::document_container {
public:
    explicit xiao_doc_container(int w, int h) {
        viewport_.x = 0;
        viewport_.y = 0;
        viewport_.width = (float)w;
        viewport_.height = (float)h;
    }

    litehtml::uint_ptr create_font(const litehtml::font_description& descr, const litehtml::document*, litehtml::font_metrics* fm) override {
        auto* fh = new font_handle;
        fh->px = descr.size > 0 ? descr.size : 16.0f;
        if (fm) {
            fm->font_size = fh->px;
            fm->height = fh->px * 1.25f;
            fm->ascent = fh->px * 0.95f;
            fm->descent = fm->height - fm->ascent;
            fm->x_height = fh->px * 0.50f;
            fm->ch_width = fh->px * 0.56f;
            fm->draw_spaces = true;
            fm->sub_shift = fh->px * 0.20f;
            fm->super_shift = fh->px * 0.20f;
        }
        return reinterpret_cast<litehtml::uint_ptr>(fh);
    }

    void delete_font(litehtml::uint_ptr hFont) override {
        auto* fh = reinterpret_cast<font_handle*>(hFont);
        delete fh;
    }

    litehtml::pixel_t text_width(const char* text, litehtml::uint_ptr hFont) override {
        auto* fh = reinterpret_cast<font_handle*>(hFont);
        float px = fh ? fh->px : 16.0f;
        int n = 0;
        while (text && text[n]) n++;
        return (litehtml::pixel_t)((float)n * px * 0.56f);
    }

    void draw_text(litehtml::uint_ptr, const char*, litehtml::uint_ptr, litehtml::web_color, const litehtml::position&) override {}

    litehtml::pixel_t pt_to_px(float pt) const override {
        return (litehtml::pixel_t)(pt * (96.0f / 72.0f));
    }

    litehtml::pixel_t get_default_font_size() const override {
        return 16;
    }

    const char* get_default_font_name() const override {
        return "sans-serif";
    }

    void draw_list_marker(litehtml::uint_ptr, const litehtml::list_marker&) override {}
    void load_image(const char*, const char*, bool) override {}

    void get_image_size(const char*, const char*, litehtml::size& sz) override {
        sz.width = 0;
        sz.height = 0;
    }

    void draw_image(litehtml::uint_ptr, const litehtml::background_layer&, const std::string&, const std::string&) override {}
    void draw_solid_fill(litehtml::uint_ptr, const litehtml::background_layer&, const litehtml::web_color&) override {}
    void draw_linear_gradient(litehtml::uint_ptr, const litehtml::background_layer&, const litehtml::background_layer::linear_gradient&) override {}
    void draw_radial_gradient(litehtml::uint_ptr, const litehtml::background_layer&, const litehtml::background_layer::radial_gradient&) override {}
    void draw_conic_gradient(litehtml::uint_ptr, const litehtml::background_layer&, const litehtml::background_layer::conic_gradient&) override {}
    void draw_borders(litehtml::uint_ptr, const litehtml::borders&, const litehtml::position&, bool) override {}

    void set_caption(const char*) override {}
    void set_base_url(const char*) override {}
    void link(const std::shared_ptr<litehtml::document>&, const litehtml::element::ptr&) override {}
    void on_anchor_click(const char*, const litehtml::element::ptr&) override {}
    void on_mouse_event(const litehtml::element::ptr&, litehtml::mouse_event) override {}
    void set_cursor(const char*) override {}

    void transform_text(litehtml::string& text, litehtml::text_transform tt) override {
        if (tt == litehtml::text_transform_uppercase) {
            for (char& c : text) c = (char)std::toupper((unsigned char)c);
        } else if (tt == litehtml::text_transform_lowercase) {
            for (char& c : text) c = (char)std::tolower((unsigned char)c);
        } else if (tt == litehtml::text_transform_capitalize) {
            bool start = true;
            for (char& c : text) {
                if (std::isspace((unsigned char)c)) {
                    start = true;
                } else if (start) {
                    c = (char)std::toupper((unsigned char)c);
                    start = false;
                }
            }
        }
    }

    void import_css(litehtml::string& text, const litehtml::string&, litehtml::string& baseurl) override {
        text.clear();
        baseurl.clear();
    }

    void set_clip(const litehtml::position&, const litehtml::border_radiuses&) override {}
    void del_clip() override {}

    void get_viewport(litehtml::position& viewport) const override {
        viewport = viewport_;
    }

    litehtml::element::ptr create_element(const char*, const litehtml::string_map&, const std::shared_ptr<litehtml::document>&) override {
        return nullptr;
    }

    void get_media_features(litehtml::media_features& media) const override {
        media.type = litehtml::media_type_screen;
        media.width = viewport_.width;
        media.height = viewport_.height;
        media.device_width = viewport_.width;
        media.device_height = viewport_.height;
        media.color = 8;
        media.color_index = 0;
        media.monochrome = 0;
        media.resolution = 96;
    }

    void get_language(litehtml::string& language, litehtml::string& culture) const override {
        language = "en";
        culture = "";
    }

private:
    litehtml::position viewport_;
};

static void copy_cap(char* dst, size_t cap, const std::string& src) {
    size_t i = 0;
    if (!dst || cap == 0) return;
    while (i + 1 < cap && i < src.size()) {
        dst[i] = src[i];
        i++;
    }
    dst[i] = 0;
}

static void fill_from_selector(const std::shared_ptr<litehtml::element>& root,
                               const char* selector,
                               int* x, int* y, int* w, int* h,
                               char* text, size_t text_cap) {
    if (!root) return;
    auto el = root->select_one(selector);
    if (!el) return;

    auto pos = el->get_placement();
    if (x) *x = (int)pos.x;
    if (y) *y = (int)pos.y;
    if (w) *w = (int)pos.width;
    if (h) *h = (int)pos.height;

    if (text && text_cap > 0) {
        litehtml::string s;
        el->get_text(s);
        copy_cap(text, text_cap, s);
    }
}

} // namespace

extern "C" int xiao_litehtml_layout(const char* html, int viewport_w, int viewport_h, xiao_litehtml_layout_result* out) {
    if (!html || !out || viewport_w <= 0 || viewport_h <= 0) return -1;

    std::memset(out, 0, sizeof(*out));

    xiao_doc_container container(viewport_w, viewport_h);
    auto doc = litehtml::document::createFromString(litehtml::estring(html), &container);
    if (!doc) return -1;

    doc->render((litehtml::pixel_t)viewport_w);

    auto root = doc->root();
    if (!root) return -1;

    fill_from_selector(root, "h1", &out->title_x, &out->title_y, &out->title_w, &out->title_h, out->title, sizeof(out->title));
    fill_from_selector(root, "p", &out->paragraph_x, &out->paragraph_y, &out->paragraph_w, &out->paragraph_h, out->paragraph, sizeof(out->paragraph));
    fill_from_selector(root, "button", &out->button_x, &out->button_y, &out->button_w, &out->button_h, out->button, sizeof(out->button));

    out->ok = 1;
    return 0;
}
