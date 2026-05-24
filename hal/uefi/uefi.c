#include "efi.h"
#include "xiao.h"

typedef unsigned char u8;
typedef unsigned short u16;

#define COM1 0x3f8

#ifdef XIAO_UEFI_X86_SERIAL
void uefi_outb(u16 port, u8 value);
u8 uefi_inb(u16 port);
#endif

static EFI_SYSTEM_TABLE *st;
static int esc_state;
static EFI_GRAPHICS_OUTPUT_PROTOCOL *gop;
static int gop_mode_ready;
int _fltused = 0;

typedef enum {
    XIAO_EFI_RESET_COLD = 0,
    XIAO_EFI_RESET_WARM = 1,
    XIAO_EFI_RESET_SHUTDOWN = 2,
    XIAO_EFI_RESET_PLATFORM_SPECIFIC = 3
} XIAO_EFI_RESET_TYPE;

typedef void (*XIAO_EFI_RESET_SYSTEM)(
    XIAO_EFI_RESET_TYPE ResetType,
    EFI_STATUS ResetStatus,
    UINTN DataSize,
    void *ResetData
);

typedef struct {
    EFI_TABLE_HEADER Hdr;
    void *GetTime;
    void *SetTime;
    void *GetWakeupTime;
    void *SetWakeupTime;
    void *SetVirtualAddressMap;
    void *ConvertPointer;
    void *GetVariable;
    void *GetNextVariableName;
    void *SetVariable;
    void *GetNextHighMonotonicCount;
    XIAO_EFI_RESET_SYSTEM ResetSystem;
    void *UpdateCapsule;
    void *QueryCapsuleCapabilities;
    void *QueryVariableInfo;
} XIAO_EFI_RUNTIME_SERVICES;

static const EFI_GUID gop_guid = {
    0x9042a9de, 0x23dc, 0x4a38, {0x96, 0xfb, 0x7a, 0xde, 0xd0, 0x80, 0x51, 0x6a}
};

typedef struct {
    u16 ScanCode;
    CHAR16 UnicodeChar;
} XIAO_EFI_INPUT_KEY;

typedef EFI_STATUS (*XIAO_EFI_READ_KEY_STROKE)(void *self, XIAO_EFI_INPUT_KEY *key);

typedef struct {
    void *Reset;
    XIAO_EFI_READ_KEY_STROKE ReadKeyStroke;
} XIAO_EFI_SIMPLE_TEXT_INPUT_PROTOCOL;

static void uefi_putc(char c) {
    if (st && st->ConOut && st->ConOut->OutputString) {
        CHAR16 out[2];
        out[0] = (CHAR16)c;
        out[1] = 0;
        st->ConOut->OutputString(st->ConOut, out);
    }
}

static void uefi_console_write(const char *data, xiao_size len) {
    xiao_size i;
    for (i = 0; i < len; i++) {
        char c = data[i];
        if (esc_state == 1) {
            if (c == '[') {
                esc_state = 2;
                continue;
            }
            esc_state = 0;
        } else if (esc_state == 2) {
            if (c == 'H') {
                if (st && st->ConOut && st->ConOut->SetCursorPosition) {
                    st->ConOut->SetCursorPosition(st->ConOut, 0, 0);
                }
                esc_state = 0;
                continue;
            }
            if (c == '2') {
                esc_state = 3;
                continue;
            }
            esc_state = 0;
        } else if (esc_state == 3) {
            if (c == 'J') {
                if (st && st->ConOut && st->ConOut->ClearScreen) {
                    st->ConOut->ClearScreen(st->ConOut);
                }
                esc_state = 0;
                continue;
            }
            esc_state = 0;
        }

        if ((u8)c == 0x1b) {
            esc_state = 1;
            continue;
        }

        uefi_putc(c);
    }
}

static void serial_init(void) {
#ifdef XIAO_UEFI_X86_SERIAL
    uefi_outb(COM1 + 1, 0x00);
    uefi_outb(COM1 + 3, 0x80);
    uefi_outb(COM1 + 0, 0x01);
    uefi_outb(COM1 + 1, 0x00);
    uefi_outb(COM1 + 3, 0x03);
    uefi_outb(COM1 + 2, 0xc7);
    uefi_outb(COM1 + 4, 0x0b);
#endif
}

#ifdef XIAO_UEFI_X86_SERIAL
static void serial_putc(char c) {
    while ((uefi_inb(COM1 + 5) & 0x20) == 0) {}
    uefi_outb(COM1, (u8)c);
}
#endif

static void uefi_serial_write(const char *data, xiao_size len) {
#ifdef XIAO_UEFI_X86_SERIAL
    xiao_size i;
    for (i = 0; i < len; i++) serial_putc(data[i]);
    uefi_console_write(data, len);
#else
    uefi_console_write(data, len);
#endif
}

static int uefi_input_read(void) {
    XIAO_EFI_INPUT_KEY key;
    XIAO_EFI_SIMPLE_TEXT_INPUT_PROTOCOL *in;
    if (!st || !st->ConIn) return -1;
    in = (XIAO_EFI_SIMPLE_TEXT_INPUT_PROTOCOL *)st->ConIn;
    if (!in->ReadKeyStroke) return -1;
    if (in->ReadKeyStroke(in, &key) != EFI_SUCCESS) return -1;
    if (key.UnicodeChar != 0) return (int)key.UnicodeChar;
    if (key.ScanCode == 0x17) return '\n';
    return -1;
}

static void uefi_wait_ms(xiao_tick ms) {
    volatile unsigned long i;
    while (ms--) {
        for (i = 0; i < 120000; i++) {}
    }
}

static void uefi_yield(void) {
}

static EFI_GRAPHICS_OUTPUT_PROTOCOL *uefi_get_gop(void) {
    if (gop) return gop;
    if (!st || !st->BootServices || !st->BootServices->LocateProtocol) return 0;
    if (st->BootServices->LocateProtocol((EFI_GUID *)&gop_guid, 0, (void **)&gop) != EFI_SUCCESS) return 0;
    return gop;
}

static UINT32 uefi_pick_best_graphics_mode(EFI_GRAPHICS_OUTPUT_PROTOCOL *ggop) {
    UINT32 i;
    UINT32 best_mode = 0;
    UINT32 best_pixels = 0;
    if (!ggop || !ggop->Mode || !ggop->QueryMode) return 0;

    best_mode = ggop->Mode->Mode;
    for (i = 0; i < ggop->Mode->MaxMode; i++) {
        UINTN info_size = 0;
        EFI_GRAPHICS_OUTPUT_MODE_INFORMATION *info = 0;
        if (ggop->QueryMode(ggop, i, &info_size, &info) == EFI_SUCCESS && info) {
            UINT32 pixels = info->HorizontalResolution * info->VerticalResolution;
            if (pixels > best_pixels) {
                best_pixels = pixels;
                best_mode = i;
            }
            if (st && st->BootServices && st->BootServices->FreePool) {
                st->BootServices->FreePool(info);
            }
        }
    }
    return best_mode;
}

static int uefi_set_graphics_mode(EFI_GRAPHICS_OUTPUT_PROTOCOL *ggop, int w, int h) {
    UINT32 i;
    if (!ggop || !ggop->Mode || !ggop->QueryMode || !ggop->SetMode) return -1;
    for (i = 0; i < ggop->Mode->MaxMode; i++) {
        UINTN info_size = 0;
        EFI_GRAPHICS_OUTPUT_MODE_INFORMATION *info = 0;
        if (ggop->QueryMode(ggop, i, &info_size, &info) == EFI_SUCCESS && info) {
            int matched = (int)info->HorizontalResolution == w && (int)info->VerticalResolution == h;
            if (st && st->BootServices && st->BootServices->FreePool) st->BootServices->FreePool(info);
            if (matched) return ggop->SetMode(ggop, i) == EFI_SUCCESS ? 0 : -1;
        }
    }
    return -1;
}

static void uefi_enter_default_graphics_mode(EFI_GRAPHICS_OUTPUT_PROTOCOL *ggop) {
    if (!ggop || !ggop->Mode || !ggop->SetMode) return;
    if (uefi_set_graphics_mode(ggop, 1280, 720) == 0) return;
    ggop->SetMode(ggop, uefi_pick_best_graphics_mode(ggop));
}

static int uefi_prepare_gop(void) {
    EFI_GRAPHICS_OUTPUT_PROTOCOL *ggop = uefi_get_gop();
    if (!ggop || !ggop->Mode || !ggop->Mode->Info) return -1;
    if (!gop_mode_ready) {
        uefi_enter_default_graphics_mode(ggop);
        gop_mode_ready = 1;
    }
    return 0;
}

static UINT32 uefi_masked_component(UINT32 v8, UINT32 mask) {
    UINT32 m = mask;
    UINT32 shift = 0;
    UINT32 bits = 0;
    while ((m & 1u) == 0u) {
        shift++;
        m >>= 1;
        if (m == 0) return 0;
    }
    while ((m & 1u) == 1u) {
        bits++;
        m >>= 1;
    }
    if (bits == 0) return 0;
    return (((v8 * ((1u << bits) - 1u)) / 255u) << shift) & mask;
}

static int uefi_color_from_rgb888(EFI_GRAPHICS_OUTPUT_MODE_INFORMATION *info, unsigned int rgb888, UINT32 *out) {
    UINT32 color = 0;
    u8 r = (u8)((rgb888 >> 16) & 0xff);
    u8 g = (u8)((rgb888 >> 8) & 0xff);
    u8 b = (u8)(rgb888 & 0xff);
    if (!info || !out) return -1;

    if (info->PixelFormat == PixelRedGreenBlueReserved8BitPerColor) {
        color = ((UINT32)b << 16) | ((UINT32)g << 8) | (UINT32)r;
    } else if (info->PixelFormat == PixelBlueGreenRedReserved8BitPerColor) {
        color = ((UINT32)r << 16) | ((UINT32)g << 8) | (UINT32)b;
    } else if (info->PixelFormat == PixelBitMask) {
        color = uefi_masked_component(r, info->PixelInformation.RedMask) |
                uefi_masked_component(g, info->PixelInformation.GreenMask) |
                uefi_masked_component(b, info->PixelInformation.BlueMask);
    } else {
        return -1;
    }
    *out = color;
    return 0;
}

static int uefi_video_size(int *w, int *h) {
    if (!w || !h || uefi_prepare_gop() != 0) return -1;
    *w = (int)gop->Mode->Info->HorizontalResolution;
    *h = (int)gop->Mode->Info->VerticalResolution;
    return 0;
}

static int uefi_video_set_mode(int w, int h) {
    if (w <= 0 || h <= 0) return -1;
    if (uefi_prepare_gop() != 0) return -1;
    return uefi_set_graphics_mode(gop, w, h);
}

static int uefi_video_draw_pixel_rgb888(int x, int y, unsigned int rgb888) {
    EFI_GRAPHICS_OUTPUT_PROTOCOL_MODE *mode;
    EFI_GRAPHICS_OUTPUT_MODE_INFORMATION *info;
    volatile UINT32 *fb;
    UINT32 color;
    if (uefi_prepare_gop() != 0) return -1;
    mode = gop->Mode;
    info = mode->Info;
    if (x < 0 || y < 0 || x >= (int)info->HorizontalResolution || y >= (int)info->VerticalResolution) return -1;
    if (info->PixelFormat == PixelBltOnly) {
        EFI_GRAPHICS_OUTPUT_BLT_PIXEL px;
        if (!gop->Blt) return -1;
        px.Red = (u8)((rgb888 >> 16) & 0xff);
        px.Green = (u8)((rgb888 >> 8) & 0xff);
        px.Blue = (u8)(rgb888 & 0xff);
        px.Reserved = 0;
        return gop->Blt(gop, &px, EfiBltVideoFill, 0, 0, (UINTN)x, (UINTN)y, 1, 1, 0) == EFI_SUCCESS ? 0 : -1;
    }
    if (uefi_color_from_rgb888(info, rgb888, &color) != 0) return -1;
    fb = (volatile UINT32 *)(UINTN)mode->FrameBufferBase;
    if (!fb) return -1;
    fb[(UINTN)y * (UINTN)info->PixelsPerScanLine + (UINTN)x] = color;
    return 0;
}

static int uefi_video_fill_rect_rgb888(int x, int y, int w, int h, unsigned int rgb888) {
    EFI_GRAPHICS_OUTPUT_PROTOCOL_MODE *mode;
    EFI_GRAPHICS_OUTPUT_MODE_INFORMATION *info;
    volatile UINT32 *fb;
    UINT32 color;
    int x0, y0, x1, y1;
    int yy, xx;
    if (w <= 0 || h <= 0) return -1;
    if (uefi_prepare_gop() != 0) return -1;
    mode = gop->Mode;
    info = mode->Info;
    x0 = x < 0 ? 0 : x;
    y0 = y < 0 ? 0 : y;
    x1 = x + w;
    y1 = y + h;
    if (x1 > (int)info->HorizontalResolution) x1 = (int)info->HorizontalResolution;
    if (y1 > (int)info->VerticalResolution) y1 = (int)info->VerticalResolution;
    if (x0 >= x1 || y0 >= y1) return -1;
    if (info->PixelFormat == PixelBltOnly) {
        EFI_GRAPHICS_OUTPUT_BLT_PIXEL px;
        if (!gop->Blt) return -1;
        px.Red = (u8)((rgb888 >> 16) & 0xff);
        px.Green = (u8)((rgb888 >> 8) & 0xff);
        px.Blue = (u8)(rgb888 & 0xff);
        px.Reserved = 0;
        return gop->Blt(gop, &px, EfiBltVideoFill, 0, 0, (UINTN)x0, (UINTN)y0, (UINTN)(x1 - x0), (UINTN)(y1 - y0), 0) == EFI_SUCCESS ? 0 : -1;
    }
    if (uefi_color_from_rgb888(info, rgb888, &color) != 0) return -1;
    fb = (volatile UINT32 *)(UINTN)mode->FrameBufferBase;
    if (!fb) return -1;

    for (yy = y0; yy < y1; yy++) {
        UINTN row = (UINTN)yy * (UINTN)info->PixelsPerScanLine;
        for (xx = x0; xx < x1; xx++) {
            fb[row + (UINTN)xx] = color;
        }
    }
    return 0;
}

#define UEFI_BLT_ROW_MAX 4096

static int uefi_video_blit_rgb888(int x, int y, int w, int h, const unsigned int *pixels, int stride) {
    EFI_GRAPHICS_OUTPUT_PROTOCOL_MODE *mode;
    EFI_GRAPHICS_OUTPUT_MODE_INFORMATION *info;
    volatile UINT32 *fb;
    int x0, y0, x1, y1;
    int sx0, sy0;
    int rw, rh;
    int yy, xx;

    if (!pixels || w <= 0 || h <= 0 || stride < w) return -1;
    if (uefi_prepare_gop() != 0) return -1;

    mode = gop->Mode;
    info = mode->Info;
    x0 = x < 0 ? 0 : x;
    y0 = y < 0 ? 0 : y;
    x1 = x + w;
    y1 = y + h;
    if (x1 > (int)info->HorizontalResolution) x1 = (int)info->HorizontalResolution;
    if (y1 > (int)info->VerticalResolution) y1 = (int)info->VerticalResolution;
    if (x0 >= x1 || y0 >= y1) return -1;

    sx0 = x0 - x;
    sy0 = y0 - y;
    rw = x1 - x0;
    rh = y1 - y0;

    if (info->PixelFormat == PixelBltOnly) {
        static EFI_GRAPHICS_OUTPUT_BLT_PIXEL blt_row[UEFI_BLT_ROW_MAX];
        if (!gop->Blt || rw > UEFI_BLT_ROW_MAX) return -1;

        for (yy = 0; yy < rh; yy++) {
            const unsigned int *src = pixels + (sy0 + yy) * stride + sx0;
            for (xx = 0; xx < rw; xx++) {
                unsigned int rgb = src[xx];
                blt_row[xx].Red = (u8)((rgb >> 16) & 0xff);
                blt_row[xx].Green = (u8)((rgb >> 8) & 0xff);
                blt_row[xx].Blue = (u8)(rgb & 0xff);
                blt_row[xx].Reserved = 0;
            }
            if (gop->Blt(gop, blt_row, EfiBltBufferToVideo, 0, 0, (UINTN)x0, (UINTN)(y0 + yy), (UINTN)rw, 1, 0) != EFI_SUCCESS) {
                return -1;
            }
        }
        return 0;
    }

    fb = (volatile UINT32 *)(UINTN)mode->FrameBufferBase;
    if (!fb) return -1;

    if (info->PixelFormat == PixelBlueGreenRedReserved8BitPerColor) {
        for (yy = 0; yy < rh; yy++) {
            const unsigned int *src = pixels + (sy0 + yy) * stride + sx0;
            UINTN row = (UINTN)(y0 + yy) * (UINTN)info->PixelsPerScanLine + (UINTN)x0;
            for (xx = 0; xx < rw; xx++) {
                fb[row + (UINTN)xx] = (UINT32)src[xx];
            }
        }
        return 0;
    }

    if (info->PixelFormat == PixelRedGreenBlueReserved8BitPerColor) {
        for (yy = 0; yy < rh; yy++) {
            const unsigned int *src = pixels + (sy0 + yy) * stride + sx0;
            UINTN row = (UINTN)(y0 + yy) * (UINTN)info->PixelsPerScanLine + (UINTN)x0;
            for (xx = 0; xx < rw; xx++) {
                unsigned int rgb = src[xx];
                fb[row + (UINTN)xx] = (UINT32)(((rgb & 0x000000ffu) << 16) | (rgb & 0x0000ff00u) | ((rgb & 0x00ff0000u) >> 16));
            }
        }
        return 0;
    }

    if (info->PixelFormat == PixelBitMask) {
        for (yy = 0; yy < rh; yy++) {
            const unsigned int *src = pixels + (sy0 + yy) * stride + sx0;
            UINTN row = (UINTN)(y0 + yy) * (UINTN)info->PixelsPerScanLine + (UINTN)x0;
            for (xx = 0; xx < rw; xx++) {
                UINT32 color;
                if (uefi_color_from_rgb888(info, src[xx], &color) != 0) return -1;
                fb[row + (UINTN)xx] = color;
            }
        }
        return 0;
    }

    return -1;
}

static int uefi_video_fill_rgb888(unsigned int rgb888) {
    int w, h;
    if (uefi_video_size(&w, &h) != 0) return -1;
    return uefi_video_fill_rect_rgb888(0, 0, w, h, rgb888);
}

static const xiao_hal uefi_hal = {
    uefi_serial_write,
    uefi_console_write,
    uefi_input_read,
    uefi_wait_ms,
    uefi_yield,
    uefi_video_fill_rgb888,
    uefi_video_draw_pixel_rgb888,
    uefi_video_fill_rect_rgb888,
    uefi_video_size,
    uefi_video_set_mode,
    XIAO_PLATFORM_PC,
    uefi_video_blit_rgb888,
};

int xiao_uefi_reboot(void) {
    XIAO_EFI_RUNTIME_SERVICES *rt;
    if (!st || !st->RuntimeServices) return -1;
    rt = (XIAO_EFI_RUNTIME_SERVICES *)st->RuntimeServices;
    if (!rt->ResetSystem) return -1;
    rt->ResetSystem(XIAO_EFI_RESET_WARM, EFI_SUCCESS, 0, 0);
    return 0;
}

int xiao_uefi_shutdown(void) {
    XIAO_EFI_RUNTIME_SERVICES *rt;
    if (!st || !st->RuntimeServices) return -1;
    rt = (XIAO_EFI_RUNTIME_SERVICES *)st->RuntimeServices;
    if (!rt->ResetSystem) return -1;
    rt->ResetSystem(XIAO_EFI_RESET_SHUTDOWN, EFI_SUCCESS, 0, 0);
    return 0;
}

EFI_STATUS efi_main(EFI_HANDLE image, EFI_SYSTEM_TABLE *system_table) {
    (void)image;
    st = system_table;
    serial_init();
    xiao_start(&uefi_hal, &xiao_image);
    return EFI_SUCCESS;
}
