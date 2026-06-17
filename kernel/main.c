#include "efi.h"

static EFI_SYSTEM_TABLE *ST;
static EFI_BOOT_SERVICES *BS;
static EFI_GRAPHICS_OUTPUT_PROTOCOL *GOP;
static EFI_SIMPLE_POINTER_PROTOCOL *Mouse;

void put_pixel(uint32_t x, uint32_t y, uint32_t color) {
    if (x >= 640 || y >= 480) return;
    uint32_t *fb = (uint32_t *)GOP->Mode->FrameBufferBase;
    // Assuming 640x480 and 32-bit pixels (BGRA or RGBA)
    fb[y * 640 + x] = color;
}

void draw_cursor(int x, int y, uint32_t color) {
    for(int i=-3; i<=3; i++) {
        for(int j=-3; j<=3; j++) {
            put_pixel(x+i, y+j, color);
        }
    }
}

EFI_STATUS EfiMain(EFI_HANDLE ImageHandle, EFI_SYSTEM_TABLE *SystemTable) {
    ST = SystemTable;
    BS = ST->BootServices;

    ST->ConOut->OutputString(ST->ConOut, L"xiaoOS Booting...\r\n");

    // 1. Locate GOP
    EFI_GUID gop_guid = EFI_GRAPHICS_OUTPUT_PROTOCOL_GUID;
    if (BS->LocateProtocol(&gop_guid, 0, (void **)&GOP) != EFI_SUCCESS) {
        ST->ConOut->OutputString(ST->ConOut, L"GOP not found!\r\n");
        return EFI_UNSUPPORTED;
    }

    // 2. Locate Mouse
    EFI_GUID mouse_guid = EFI_SIMPLE_POINTER_PROTOCOL_GUID;
    if (BS->LocateProtocol(&mouse_guid, 0, (void **)&Mouse) == EFI_SUCCESS) {
        Mouse->Reset(Mouse, 1);
        ST->ConOut->OutputString(ST->ConOut, L"Mouse initialized.\r\n");
    } else {
        ST->ConOut->OutputString(ST->ConOut, L"Mouse not found.\r\n");
    }

    // 3. Clear Screen (Background)
    uint32_t *fb = (uint32_t *)GOP->Mode->FrameBufferBase;
    for(uint64_t i=0; i<640*480; i++) fb[i] = 0xFF123456;

    int mx = 320, my = 240;
    draw_cursor(mx, my, 0xFFFFFFFF);

    ST->ConOut->OutputString(ST->ConOut, L"Press 'q' to exit loop.\r\n");

    while(1) {
        // Keyboard handling
        EFI_INPUT_KEY key;
        if (ST->ConIn->ReadKeyStroke(ST->ConIn, &key) == EFI_SUCCESS) {
            if (key.UnicodeChar == 'q') break;
            uint16_t str[2] = {key.UnicodeChar, 0};
            ST->ConOut->OutputString(ST->ConOut, str);
        }

        // Mouse handling
        if (Mouse) {
            EFI_SIMPLE_POINTER_STATE state;
            if (Mouse->GetState(Mouse, &state) == EFI_SUCCESS) {
                if (state.RelativeMovementX != 0 || state.RelativeMovementY != 0) {
                    draw_cursor(mx, my, 0xFF123456); // Erase
                    mx += state.RelativeMovementX / 2;
                    my += state.RelativeMovementY / 2;
                    if (mx < 0) mx = 0; if (mx >= 640) mx = 639;
                    if (my < 0) my = 0; if (my >= 480) my = 479;
                    draw_cursor(mx, my, 0xFFFFFFFF); // Draw
                }
            }
        }

        BS->Stall(10000); // 10ms
    }

    ST->ConOut->OutputString(ST->ConOut, L"Exiting...\r\n");
    return EFI_SUCCESS;
}
