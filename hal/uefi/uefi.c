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

static void uefi_console_write(const char *data, xiao_size len) {
    CHAR16 buf[96];
    xiao_size i = 0;
    while (i < len) {
        xiao_size n = 0;
        while (i < len && n + 1 < sizeof(buf) / sizeof(buf[0])) {
            buf[n++] = (CHAR16)data[i++];
        }
        buf[n] = 0;
        if (st && st->ConOut && st->ConOut->OutputString) {
            st->ConOut->OutputString(st->ConOut, buf);
        }
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

static void uefi_wait_ms(xiao_tick ms) {
    volatile unsigned long i;
    while (ms--) {
        for (i = 0; i < 120000; i++) {}
    }
}

static void uefi_yield(void) {
}

static const xiao_hal uefi_hal = {
    uefi_serial_write,
    uefi_console_write,
    uefi_wait_ms,
    uefi_yield,
};

EFI_STATUS efi_main(EFI_HANDLE image, EFI_SYSTEM_TABLE *system_table) {
    (void)image;
    st = system_table;
    serial_init();
    xiao_start(&uefi_hal, &xiao_image);
    return EFI_SUCCESS;
}
