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

static void uefi_enter_best_graphics_mode(EFI_GRAPHICS_OUTPUT_PROTOCOL *ggop) {
    UINT32 i;
    UINT32 best_mode;
    UINT32 best_pixels;
    if (!ggop || !ggop->Mode || !ggop->QueryMode || !ggop->SetMode) return;

    best_mode = ggop->Mode->Mode;
    best_pixels = 0;
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
    ggop->SetMode(ggop, best_mode);
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

static int uefi_video_fill_rgb888(unsigned int rgb888) {
    EFI_GRAPHICS_OUTPUT_PROTOCOL *ggop = uefi_get_gop();
    EFI_GRAPHICS_OUTPUT_PROTOCOL_MODE *mode;
    EFI_GRAPHICS_OUTPUT_MODE_INFORMATION *info;
    volatile UINT32 *fb;
    UINT32 x, y;
    UINT32 color = 0;
    u8 r = (u8)((rgb888 >> 16) & 0xff);
    u8 g = (u8)((rgb888 >> 8) & 0xff);
    u8 b = (u8)(rgb888 & 0xff);

    if (!ggop || !ggop->Mode || !ggop->Mode->Info) return -1;
    uefi_enter_best_graphics_mode(ggop);
    mode = ggop->Mode;
    info = mode->Info;
    fb = (volatile UINT32 *)(UINTN)mode->FrameBufferBase;
    if (!fb) return -1;

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

    for (y = 0; y < info->VerticalResolution; y++) {
        UINTN row = (UINTN)y * (UINTN)info->PixelsPerScanLine;
        for (x = 0; x < info->HorizontalResolution; x++) {
            fb[row + x] = color;
        }
    }
    return 0;
}

static const xiao_hal uefi_hal = {
    uefi_serial_write,
    uefi_console_write,
    uefi_input_read,
    uefi_wait_ms,
    uefi_yield,
    uefi_video_fill_rgb888,
    XIAO_PLATFORM_PC,
};

EFI_STATUS efi_main(EFI_HANDLE image, EFI_SYSTEM_TABLE *system_table) {
    (void)image;
    st = system_table;
    serial_init();
    xiao_start(&uefi_hal, &xiao_image);
    return EFI_SUCCESS;
}
