#include "xiao.h"

// Basic PS/2 Ports
#define PS2_DATA 0x60
#define PS2_STATUS 0x64
#define PS2_CMD 0x64

// Keyboard state
static int ctrl_pressed = 0;
static int super_pressed = 0;

static void outb(unsigned short port, unsigned char value) {
    __asm__ __volatile__("outb %0, %1" : : "a"(value), "Nd"(port));
}

static unsigned char inb(unsigned short port) {
    unsigned char value;
    __asm__ __volatile__("inb %1, %0" : "=a"(value) : "Nd"(port));
    return value;
}

static void ps2_wait_write(void) {
    while (inb(PS2_STATUS) & 2) {}
}

static void ps2_wait_read(void) {
    while (!(inb(PS2_STATUS) & 1)) {}
}

int grub_input_read(void) {
    static unsigned char kbd_us[128] = {
        0,  27, '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '=', '\b',
        '\t', 'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p', '[', ']', '\n',
        0, 'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', ';', '\'', '`', 0, '\\',
        'z', 'x', 'c', 'v', 'b', 'n', 'm', ',', '.', '/', 0, '*', 0, ' '
    };

    if (inb(PS2_STATUS) & 1) {
        unsigned char b = inb(PS2_DATA);
        if (b == 0x1d) { ctrl_pressed = 1; return -1; }
        if (b == 0x9d) { ctrl_pressed = 0; return -1; }
        if (b == 0x5b) { super_pressed = 1; return -1; }
        if (b == 0xdb) { super_pressed = 0; return -1; }

        if (b < 128 && kbd_us[b]) return (int)kbd_us[b];
    }
    return -1;
}

int grub_get_ctrl(void) {
    return ctrl_pressed || super_pressed;
}

// Mouse state
static int mouse_x = 40;
static int mouse_y = 12;
static int mouse_btns = 0;
static int mouse_cycle = 0;
static unsigned char mouse_bytes[3];

void grub_mouse_init(void) {
    ps2_wait_write();
    outb(PS2_CMD, 0xAD); // Disable KBD
    ps2_wait_write();
    outb(PS2_CMD, 0xA7); // Disable Mouse
    
    // Enable IRQ12 in controller command byte
    ps2_wait_write();
    outb(PS2_CMD, 0x20);
    ps2_wait_read();
    unsigned char status = inb(PS2_DATA) | 2;
    ps2_wait_write();
    outb(PS2_CMD, 0x60);
    ps2_wait_write();
    outb(PS2_DATA, status);
    
    ps2_wait_write();
    outb(PS2_CMD, 0xA8); // Enable Mouse port

    // Set default and enable reporting
    ps2_wait_write();
    outb(PS2_CMD, 0xD4);
    ps2_wait_write();
    outb(PS2_DATA, 0xF6); // Set default
    inb(PS2_DATA); // ACK
    
    ps2_wait_write();
    outb(PS2_CMD, 0xD4);
    ps2_wait_write();
    outb(PS2_DATA, 0xF4); // Enable reporting
    inb(PS2_DATA); // ACK
    
    ps2_wait_write();
    outb(PS2_CMD, 0xAE); // Re-enable KBD
}

void grub_mouse_poll(void) {
    int limit = 20;
    while (limit-- > 0 && (inb(PS2_STATUS) & 1)) {
        unsigned char b = inb(PS2_DATA);
        if (mouse_cycle == 0 && !(b & 0x08)) continue;

        mouse_bytes[mouse_cycle++] = b;
        if (mouse_cycle == 3) {
            mouse_cycle = 0;
            if (mouse_bytes[0] & 0x80 || mouse_bytes[0] & 0x40) continue;
            
            int dx = (int)(signed char)mouse_bytes[1];
            int dy = (int)(signed char)mouse_bytes[2];
            
            mouse_x += dx;
            mouse_y -= dy;
            
            if (mouse_x < 0) mouse_x = 0;
            if (mouse_y < 0) mouse_y = 0;
            // Assuming 80x25 text mode for now
            if (mouse_x >= 80) mouse_x = 79;
            if (mouse_y >= 25) mouse_y = 24;
            mouse_btns = mouse_bytes[0] & 0x07;
        }
    }
}

int grub_mouse_get(xiao_mouse_state *out) {
    grub_mouse_poll();
    if (out) {
        out->x = mouse_x;
        out->y = mouse_y;
        out->buttons = mouse_btns;
    }
    return 0;
}
