#ifndef XIAO_H
#define XIAO_H

typedef unsigned long xiao_size;
typedef unsigned long xiao_tick;

typedef struct xiao_env xiao_env;
typedef int (*xiao_app_main)(xiao_env *env);

typedef struct {
    const char *name;
    xiao_app_main main;
} xiao_app;

typedef struct {
    void (*serial_write)(const char *data, xiao_size len);
    void (*console_write)(const char *data, xiao_size len);
    void (*wait_ms)(xiao_tick ms);
    void (*yield)(void);
} xiao_hal;

struct xiao_env {
    const xiao_hal *hal;
};

typedef struct {
    const char *boot_text;
    const xiao_app *apps;
    xiao_size app_count;
} xiao_boot_image;

void xiao_start(const xiao_hal *hal, const xiao_boot_image *image);
void xiao_serial_print(xiao_env *env, const char *text);
void xiao_console_print(xiao_env *env, const char *text);
void xiao_wait(xiao_env *env, xiao_tick ms);
void xiao_yield(xiao_env *env);

extern const xiao_boot_image xiao_image;

#endif
