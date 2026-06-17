#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include "limine.h"
#include "efi.h"

// Limine requests
__attribute__((used, section(".limine_requests_start")))
LIMINE_REQUESTS_START_MARKER

__attribute__((used, section(".limine_requests")))
static volatile struct limine_framebuffer_request framebuffer_request = {
    .id = LIMINE_FRAMEBUFFER_REQUEST,
    .revision = 0
};

__attribute__((used, section(".limine_requests")))
static volatile struct limine_efi_system_table_request efi_request = {
    .id = LIMINE_EFI_SYSTEM_TABLE_REQUEST,
    .revision = 0
};

__attribute__((used, section(".limine_requests_end")))
LIMINE_REQUESTS_END_MARKER

static EFI_SYSTEM_TABLE *ST;
static EFI_BOOT_SERVICES *BS;
static EFI_ABSOLUTE_POINTER_PROTOCOL *AbsMouse;

static uint32_t *fb_addr;
static uint32_t fb_width;
static uint32_t fb_height;
static uint32_t fb_stride;

void hcf(void) {
    for (;;) {
#if defined(__x86_64__) || defined(__i386__)
        asm ("hlt");
#elif defined(__aarch64__)
        asm ("wfi");
#endif
    }
}

void print(const uint16_t *str) {
    if (ST && ST->ConOut) ST->ConOut->OutputString(ST->ConOut, (uint16_t *)str);
}

void draw_cursor(int x, int y, uint32_t color) {
    if (!fb_addr) return;
    for(int i=-5; i<=5; i++) {
        for(int j=-5; j<=5; j++) {
            if (i*i + j*j < 25) {
                int px = x+i, py = y+j;
                if (px >= 0 && px < (int)fb_width && py >= 0 && py < (int)fb_height) {
                    fb_addr[py * fb_stride + px] = color;
                }
            }
        }
    }
}

void kmain(void) {
    // 1. Get EFI System Table
    if (efi_request.response == NULL) hcf();
    ST = (EFI_SYSTEM_TABLE *)(uintptr_t)efi_request.response->address;
    BS = ST->BootServices;

    print(L"xiaoOS (Limine Protocol) Starting...\r\n");

    // 2. Get Framebuffer
    if (framebuffer_request.response == NULL || framebuffer_request.response->framebuffer_count < 1) {
        print(L"No Framebuffer\r\n");
        hcf();
    }
    struct limine_framebuffer *fb = framebuffer_request.response->framebuffers[0];
    fb_addr = (uint32_t *)fb->address;
    fb_width = (uint32_t)fb->width;
    fb_height = (uint32_t)fb->height;
    fb_stride = (uint32_t)(fb->pitch / 4);

    // 3. Absolute Pointer
    EFI_GUID abs_guid = EFI_ABSOLUTE_POINTER_PROTOCOL_GUID;
    if (BS->LocateProtocol(&abs_guid, 0, (void **)&AbsMouse) == EFI_SUCCESS) {
        AbsMouse->Reset(AbsMouse, 0);
        print(L"Mouse OK\r\n");
    }

    // Clear Screen
    for (uint32_t y = 0; y < fb_height; y++) {
        for (uint32_t x = 0; x < fb_width; x++) {
            fb_addr[y * fb_stride + x] = 0xFF123456;
        }
    }

    int mx = fb_width / 2;
    int my = fb_height / 2;
    draw_cursor(mx, my, 0xFFFFFFFF);

    while (1) {
        // Heartbeat
        static uint32_t count = 0;
        fb_addr[0] = (count++ % 20 < 10) ? 0xFFFFFFFF : 0x00000000;

        // Keyboard
        EFI_INPUT_KEY key;
        if (ST->ConIn->ReadKeyStroke(ST->ConIn, &key) == EFI_SUCCESS) {
            if (key.UnicodeChar == 'q') break;
            if (key.UnicodeChar != 0) {
                uint16_t str[2] = {key.UnicodeChar, 0};
                print(str);
            }
        }

        // Mouse
        if (AbsMouse && AbsMouse->Mode && AbsMouse->Mode->AbsoluteMaxX > 0 && AbsMouse->Mode->AbsoluteMaxY > 0) {
            EFI_ABSOLUTE_POINTER_STATE state;
            if (AbsMouse->GetState(AbsMouse, &state) == EFI_SUCCESS) {
                draw_cursor(mx, my, 0xFF123456);
                mx = (int)(state.CurrentX * (uint64_t)fb_width / AbsMouse->Mode->AbsoluteMaxX);
                my = (int)(state.CurrentY * (uint64_t)fb_height / AbsMouse->Mode->AbsoluteMaxY);
                draw_cursor(mx, my, 0xFFFFFFFF);
            }
        }

        BS->Stall(10000);
    }

    print(L"Done.\r\n");
    hcf();
}
