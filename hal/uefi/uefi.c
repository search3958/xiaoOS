#include "efi.h"
#include "xiao.h"

#ifndef NULL
#define NULL ((void*)0)
#endif

typedef unsigned char u8;
typedef unsigned short u16;
typedef unsigned int u32;

#define COM1 0x3f8

#ifdef XIAO_UEFI_X86_SERIAL
void uefi_outb(u16 port, u8 value);
u8 uefi_inb(u16 port);
#endif

static EFI_SYSTEM_TABLE *st;
static EFI_HANDLE g_image;
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

typedef void (EFIAPI *XIAO_EFI_RESET_SYSTEM)(
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

typedef enum {
    AllHandles,
    ByRegisterNotify,
    ByProtocol
} EFI_LOCATE_SEARCH_TYPE;

static const EFI_GUID device_path_guid = {
    0x09576e91, 0x6d3f, 0x11d2, {0x8e, 0x39, 0x00, 0xa0, 0xc9, 0x69, 0x72, 0x3b}
};

static const EFI_GUID gop_guid = {
    0x9042a9de, 0x23dc, 0x4a38, {0x96, 0xfb, 0x7a, 0xde, 0xd0, 0x80, 0x51, 0x6a}
};

static const EFI_GUID pointer_guid = {
    0x31878c87, 0x0b75, 0x11d5, {0x9a, 0x4f, 0x00, 0x90, 0x27, 0x3f, 0xc1, 0x4d}
};

/* EFI_ABSOLUTE_POINTER_PROTOCOL GUID {8D59D32B-C655-4AE9-9B15-F25904992A43} */
static const EFI_GUID abs_pointer_guid = {
    0x8d59d32b, 0xc655, 0x4ae9, {0x9b, 0x15, 0xf2, 0x59, 0x04, 0x99, 0x2a, 0x43}
};

static EFI_SIMPLE_POINTER_PROTOCOL *pointer_proto;
static EFI_ABSOLUTE_POINTER_PROTOCOL *abs_pointer_proto;
static int mouse_x, mouse_y;
static int mouse_btns;
static int mouse_initialized;

static int uefi_video_size(int *w, int *h);
static int uefi_set_mode(int w, int h);

typedef struct {
    u16 ScanCode;
    CHAR16 UnicodeChar;
} XIAO_EFI_INPUT_KEY;

typedef EFI_STATUS (EFIAPI *XIAO_EFI_READ_KEY_STROKE)(void *self, XIAO_EFI_INPUT_KEY *key);

typedef struct {
    void *Reset;
    XIAO_EFI_READ_KEY_STROKE ReadKeyStroke;
} XIAO_EFI_SIMPLE_TEXT_INPUT_PROTOCOL;

static void uefi_putc(char c) {
    if (st && st->ConOut && st->ConOut->OutputString) {
        CHAR16 out[3];
        int i = 0;
        if (c == '\n') {
            out[i++] = (CHAR16)'\r';
        }
        out[i++] = (CHAR16)c;
        out[i++] = 0;
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

static void serial_putc(char c) {
#ifdef XIAO_UEFI_X86_SERIAL
    unsigned long timeout = 1000000;
    while ((uefi_inb(COM1 + 5) & 0x20) == 0 && timeout--) {}
    if (timeout > 0) uefi_outb(COM1, (u8)c);
#endif
}

static void uefi_serial_write(const char *data, xiao_size len) {
    xiao_size i;
    for (i = 0; i < len; i++) serial_putc(data[i]);
    uefi_console_write(data, len);
}

static int uefi_mouse_get(xiao_mouse_state *out);

static int uefi_input_read(void) {
    XIAO_EFI_INPUT_KEY key;
    XIAO_EFI_SIMPLE_TEXT_INPUT_PROTOCOL *in;
    
    // Periodically poll mouse so cursor moves during input waits
    uefi_mouse_get(NULL);
    
    if (!st || !st->ConIn) return -1;
    in = (XIAO_EFI_SIMPLE_TEXT_INPUT_PROTOCOL *)st->ConIn;
    if (!in->ReadKeyStroke) return -1;
    
    EFI_STATUS status = in->ReadKeyStroke(in, &key);
    if (status == EFI_SUCCESS) {
        if (key.UnicodeChar != 0) return (int)key.UnicodeChar;
        if (key.ScanCode == 0x17) return '\n';
        if (key.ScanCode == 0x01) return 0x10; // Ctrl+P / Up (approx)
    }
    return -1;
}

static void uefi_wait_ms(xiao_tick ms) {
    if (st && st->BootServices && st->BootServices->Stall) {
        typedef EFI_STATUS (EFIAPI *EFI_STALL)(UINTN Microseconds);
        ((EFI_STALL)st->BootServices->Stall)((UINTN)ms * 1000);
    } else {
        volatile unsigned long i;
        while (ms--) {
            for (i = 0; i < 120000; i++) {}
        }
    }
}

static void uefi_yield(void) {
}

static const u8 cursor_mask[16] = {
    0x80, 0xC0, 0xE0, 0xF0, 0xF8, 0xFC, 0xFE, 0xFF,
    0xF0, 0xD8, 0x8C, 0x0C, 0x06, 0x06, 0x00, 0x00
};

static u32 cursor_bg[16 * 16];
static int last_mouse_x = -1, last_mouse_y = -1;

static int uefi_video_draw_pixel_rgb888(int x, int y, unsigned int rgb888);
static void draw_cursor(int x, int y, int draw);

static void uefi_connect_all(void) {
    EFI_HANDLE *handles = NULL;
    UINTN count = 0;
    // 0 = AllHandles
    EFI_STATUS status = st->BootServices->LocateHandleBuffer(0, NULL, NULL, &count, &handles);
    if (status == EFI_SUCCESS) {
        for (UINTN i = 0; i < count; i++) {
            st->BootServices->ConnectController(handles[i], NULL, NULL, 1);
        }
        st->BootServices->FreePool(handles);
    }
}

static EFI_STATUS locate_and_open_pointer(EFI_GUID *guid, void **out_proto, EFI_HANDLE image) {
    EFI_HANDLE *handles = NULL;
    UINTN count = 0;
    // 2 = ByProtocol
    EFI_STATUS status = st->BootServices->LocateHandleBuffer(2, guid, NULL, &count, &handles);
    
    if (status == EFI_SUCCESS) {
        for (UINTN i = 0; i < count; i++) {
            status = st->BootServices->OpenProtocol(handles[i], guid, out_proto, image, NULL, 0x00000001u);
            if (status == EFI_SUCCESS) {
                st->BootServices->FreePool(handles);
                return EFI_SUCCESS;
            }
        }
        st->BootServices->FreePool(handles);
    }
    return 0x800000000000000EULL;
}

static int uefi_mouse_get(xiao_mouse_state *out) {
    int sw = 1280, sh = 720;
    uefi_video_size(&sw, &sh);
    if (sw <= 0) sw = 1280;
    if (sh <= 0) sh = 720;

    if (!mouse_initialized) {
        static const char msg0[] = "UEFI: Discovering hardware...\r\n";
        uefi_serial_write(msg0, sizeof(msg0) - 1);
        
        uefi_connect_all();

        locate_and_open_pointer((EFI_GUID *)&pointer_guid, (void **)&pointer_proto, g_image);
        locate_and_open_pointer((EFI_GUID *)&abs_pointer_guid, (void **)&abs_pointer_proto, g_image);

        if (pointer_proto) {
            if (pointer_proto->Reset) pointer_proto->Reset(pointer_proto, 1);
            static const char msg1[] = "UEFI: SimplePointer active\r\n";
            uefi_serial_write(msg1, sizeof(msg1) - 1);
        }
        if (abs_pointer_proto) {
            if (abs_pointer_proto->Reset) abs_pointer_proto->Reset(abs_pointer_proto, 1);
            static const char msg2[] = "UEFI: AbsolutePointer active\r\n";
            uefi_serial_write(msg2, sizeof(msg2) - 1);
        }
        
        mouse_x = sw / 2;
        mouse_y = sh / 2;
        mouse_btns = 0;
        mouse_initialized = 1;
        last_mouse_x = -1; last_mouse_y = -1;
    }

    int moved = 0;
    if (pointer_proto) {
        if (pointer_proto->WaitForInput && st->BootServices->CheckEvent(pointer_proto->WaitForInput) == 0) {
            EFI_SIMPLE_POINTER_STATE state;
            if (pointer_proto->GetState(pointer_proto, &state) == 0) {
                int dx = (int)state.RelativeMovementX;
                int dy = (int)state.RelativeMovementY;
                if (pointer_proto->Mode && pointer_proto->Mode->ResolutionX > 0) {
                    dx = (dx * 10) / (int)pointer_proto->Mode->ResolutionX;
                    dy = (dy * 10) / (int)pointer_proto->Mode->ResolutionY;
                    if (dx == 0 && state.RelativeMovementX != 0) dx = state.RelativeMovementX > 0 ? 1 : -1;
                    if (dy == 0 && state.RelativeMovementY != 0) dy = state.RelativeMovementY > 0 ? 1 : -1;
                }
                mouse_x += dx; mouse_y += dy;
                mouse_btns = (state.LeftButton ? 1 : 0) | (state.RightButton ? 2 : 0);
                moved = 1;
            }
        }
    } 
    
    if (abs_pointer_proto) {
        if (abs_pointer_proto->WaitForInput && st->BootServices->CheckEvent(abs_pointer_proto->WaitForInput) == 0) {
            EFI_ABSOLUTE_POINTER_STATE state;
            if (abs_pointer_proto->GetState(abs_pointer_proto, &state) == 0) {
                EFI_ABSOLUTE_POINTER_MODE *mode = abs_pointer_proto->Mode;
                if (mode && (mode->AbsoluteMaxX - mode->AbsoluteMinX) > 0) {
                    mouse_x = (int)((UINTN)sw * (state.CurrentX - mode->AbsoluteMinX) / (mode->AbsoluteMaxX - mode->AbsoluteMinX));
                    mouse_y = (int)((UINTN)sh * (state.CurrentY - mode->AbsoluteMinY) / (mode->AbsoluteMaxY - mode->AbsoluteMinY));
                }
                mouse_btns = (state.ActiveButtons & 0x01 ? 1 : 0) | (state.ActiveButtons & 0x02 ? 2 : 0);
                moved = 1;
            }
        }
    }

    if (mouse_x < 0) mouse_x = 0;
    if (mouse_y < 0) mouse_y = 0;
    if (mouse_x >= sw) mouse_x = sw - 1;
    if (mouse_y >= sh) mouse_y = sh - 1;

    if (moved || (last_mouse_x == -1)) {
        draw_cursor(last_mouse_x, last_mouse_y, 0); 
        draw_cursor(mouse_x, mouse_y, 1);           
        last_mouse_x = mouse_x;
        last_mouse_y = mouse_y;
    }

    if (out) { out->x = mouse_x; out->y = mouse_y; out->buttons = mouse_btns; }
    return 0;
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
    UINT32 exact_mode = (UINT32)-1;
    UINT32 fallback_mode = (UINT32)-1;
    UINT32 fallback_pixels = 0;
    unsigned long long fallback_score = ~0ull;
    if (!ggop || !ggop->Mode || !ggop->QueryMode || !ggop->SetMode) return -1;
    for (i = 0; i < ggop->Mode->MaxMode; i++) {
        UINTN info_size = 0;
        EFI_GRAPHICS_OUTPUT_MODE_INFORMATION *info = 0;
        if (ggop->QueryMode(ggop, i, &info_size, &info) == EFI_SUCCESS && info) {
            UINT32 mw = info->HorizontalResolution;
            UINT32 mh = info->VerticalResolution;
            if ((int)mw == w && (int)mh == h) {
                exact_mode = i;
            } else {
                UINT32 reqw = w > 0 ? (UINT32)w : 0;
                UINT32 reqh = h > 0 ? (UINT32)h : 0;
                UINT32 dw = mw > reqw ? mw - reqw : reqw - mw;
                UINT32 dh = mh > reqh ? mh - reqh : reqh - mh;
                unsigned long long score = (unsigned long long)dw + (unsigned long long)dh;
                UINT32 pixels = mw * mh;
                if (score < fallback_score || (score == fallback_score && pixels > fallback_pixels)) {
                    fallback_score = score;
                    fallback_pixels = pixels;
                    fallback_mode = i;
                }
            }
            if (st && st->BootServices && st->BootServices->FreePool) {
                st->BootServices->FreePool(info);
            }
        }
    }
    if (exact_mode != (UINT32)-1) {
        if (ggop->SetMode(ggop, exact_mode) == EFI_SUCCESS) return 0;
    }
    if (fallback_mode != (UINT32)-1) {
        if (ggop->SetMode(ggop, fallback_mode) == EFI_SUCCESS) return 0;
    }
    return -1;
}

static void uefi_enter_default_graphics_mode(EFI_GRAPHICS_OUTPUT_PROTOCOL *ggop) {
    if (!ggop || !ggop->Mode || !ggop->SetMode) return;
    if (uefi_set_mode(1280, 720) == 0) return;
    ggop->SetMode(ggop, uefi_pick_best_graphics_mode(ggop));
}

static int uefi_prepare_gop(void) {
    EFI_GRAPHICS_OUTPUT_PROTOCOL *ggop = uefi_get_gop();
    if (!ggop || !ggop->Mode || !ggop->Mode->Info) return -1;
    if (!gop_mode_ready) {
        gop_mode_ready = 1;
        uefi_enter_default_graphics_mode(ggop);
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
    UINT32 r = (rgb888 >> 16) & 0xff;
    UINT32 g = (rgb888 >> 8) & 0xff;
    UINT32 b = rgb888 & 0xff;
    UINT32 color = 0;

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

static int uefi_set_mode(int w, int h) {
    if (w <= 0 || h <= 0) return -1;
    if (uefi_prepare_gop() != 0) return -1;
    return uefi_set_graphics_mode(gop, w, h);
}

static void draw_cursor(int x, int y, int draw) {
    int sw, sh;
    if (uefi_video_size(&sw, &sh) != 0) return;
    if (x < 0 || y < 0 || x >= sw || y >= sh) return;

    int bw = 16, bh = 16;
    if (x + bw > sw) bw = sw - x;
    if (y + bh > sh) bh = sh - y;
    if (bw <= 0 || bh <= 0) return;

    int i, j;
    if (draw) {
        if (gop->Blt) gop->Blt(gop, (EFI_GRAPHICS_OUTPUT_BLT_PIXEL *)cursor_bg, 1, (UINTN)x, (UINTN)y, 0, 0, (UINTN)bw, (UINTN)bh, 0);
        for (i = 0; i < bh; i++) {
            for (j = 0; j < bw; j++) {
                if (cursor_mask[i] & (1 << (7 - (j / 2)))) uefi_video_draw_pixel_rgb888(x + j, y + i, 0xFFFFFFFF);
            }
        }
    } else {
        if (gop->Blt) gop->Blt(gop, (EFI_GRAPHICS_OUTPUT_BLT_PIXEL *)cursor_bg, 2, 0, 0, (UINTN)x, (UINTN)y, (UINTN)bw, (UINTN)bh, 0);
    }
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
        return gop->Blt(gop, &px, 0, 0, 0, (UINTN)x, (UINTN)y, 1, 1, 0) == 0 ? 0 : -1;
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
    if (info->PixelFormat == PixelBltOnly) {
        EFI_GRAPHICS_OUTPUT_BLT_PIXEL px;
        if (!gop->Blt) return -1;
        px.Red = (u8)((rgb888 >> 16) & 0xff);
        px.Green = (u8)((rgb888 >> 8) & 0xff);
        px.Blue = (u8)(rgb888 & 0xff);
        px.Reserved = 0;
        return gop->Blt(gop, &px, 0, 0, 0, (UINTN)x, (UINTN)y, (UINTN)w, (UINTN)h, 0) == 0 ? 0 : -1;
    }
    if (uefi_color_from_rgb888(info, rgb888, &color) != 0) return -1;
    fb = (volatile UINT32 *)(UINTN)mode->FrameBufferBase;
    if (!fb) return -1;
    x0 = x < 0 ? 0 : x;
    y0 = y < 0 ? 0 : y;
    x1 = (x + w) > (int)info->HorizontalResolution ? (int)info->HorizontalResolution : (x + w);
    y1 = (y + h) > (int)info->VerticalResolution ? (int)info->VerticalResolution : (y + h);
    for (yy = y0; yy < y1; yy++) {
        for (xx = x0; xx < x1; xx++) {
            fb[(UINTN)yy * (UINTN)info->PixelsPerScanLine + (UINTN)xx] = color;
        }
    }
    return 0;
}

static int uefi_video_blit_rgb888(int x, int y, int w, int h, const unsigned int *pixels, int stride) {
    EFI_GRAPHICS_OUTPUT_PROTOCOL_MODE *mode;
    EFI_GRAPHICS_OUTPUT_MODE_INFORMATION *info;
    volatile UINT32 *fb;
    int x0, y0, x1, y1;
    int yy, xx;
    if (w <= 0 || h <= 0 || !pixels) return -1;
    if (uefi_prepare_gop() != 0) return -1;
    mode = gop->Mode;
    info = mode->Info;
    fb = (volatile UINT32 *)(UINTN)mode->FrameBufferBase;
    if (!fb) return -1;
    x0 = x < 0 ? 0 : x;
    y0 = y < 0 ? 0 : y;
    x1 = (x + w) > (int)info->HorizontalResolution ? (int)info->HorizontalResolution : (x + w);
    y1 = (y + h) > (int)info->VerticalResolution ? (int)info->VerticalResolution : (y + h);
    for (yy = y0; yy < y1; yy++) {
        for (xx = x0; xx < x1; xx++) {
            UINT32 color;
            unsigned int rgb888 = pixels[(yy - y) * stride + (xx - x)];
            if (uefi_color_from_rgb888(info, rgb888, &color) == 0) {
                fb[(UINTN)yy * (UINTN)info->PixelsPerScanLine + (UINTN)xx] = color;
            }
        }
    }
    return 0;
}

static int uefi_video_fill_rgb888(unsigned int rgb888) {
    int w, h;
    if (uefi_video_size(&w, &h) != 0) return -1;
    return uefi_video_fill_rect_rgb888(0, 0, w, h, rgb888);
}

static void uefi_mouse_reset(void) {
    if (pointer_proto && pointer_proto->Reset) {
        pointer_proto->Reset(pointer_proto, 1);
    }
    if (abs_pointer_proto && abs_pointer_proto->Reset) {
        abs_pointer_proto->Reset(abs_pointer_proto, 1);
    }
}

static void uefi_mouse_move(int dx, int dy) {
    (void)dx; (void)dy;
}

static void uefi_mouse_set_buttons(int buttons) {
    mouse_btns = buttons;
}

static int uefi_get_ctrl(void) {
    return 0;
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
    uefi_set_mode,
    XIAO_PLATFORM_PC,
    uefi_video_blit_rgb888,
    uefi_mouse_get,
    uefi_mouse_reset,
    uefi_mouse_move,
    uefi_mouse_set_buttons,
    uefi_get_ctrl,
};

int xiao_uefi_reboot(void) {
    XIAO_EFI_RUNTIME_SERVICES *rt;
    if (!st || !st->RuntimeServices) return -1;
    rt = (XIAO_EFI_RUNTIME_SERVICES *)st->RuntimeServices;
    if (!rt->ResetSystem) return -1;
    rt->ResetSystem(XIAO_EFI_RESET_WARM, 0, 0, 0);
    return 0;
}

int xiao_uefi_shutdown(void) {
    XIAO_EFI_RUNTIME_SERVICES *rt;
    if (!st || !st->RuntimeServices) return -1;
    rt = (XIAO_EFI_RUNTIME_SERVICES *)st->RuntimeServices;
    if (!rt->ResetSystem) return -1;
    rt->ResetSystem(XIAO_EFI_RESET_SHUTDOWN, 0, 0, 0);
    return 0;
}

EFI_STATUS EFIAPI efi_main(EFI_HANDLE image, EFI_SYSTEM_TABLE *system_table) {
    st = system_table;
    g_image = image;
    serial_init();
    xiao_start(&uefi_hal, &xiao_image);
    return 0;
}
