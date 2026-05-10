#ifndef XIAO_H
#define XIAO_H

typedef unsigned long xiao_size;
typedef unsigned long xiao_tick;

typedef struct xiao_env xiao_env;
typedef int (*xiao_app_main)(xiao_env *env);

typedef struct {
    const char *name;
    const char *data;
    xiao_size size;
} xiao_file;

typedef struct {
    const char *name;
    xiao_app_main main;
} xiao_app;

typedef struct {
    const char *from;
    int argc;
    const char **argv;
} xiao_ipc_message;

typedef struct {
    void (*serial_write)(const char *data, xiao_size len);
    void (*console_write)(const char *data, xiao_size len);
    int (*input_read)(void);
    void (*wait_ms)(xiao_tick ms);
    void (*yield)(void);
} xiao_hal;

struct xiao_env {
    const xiao_hal *hal;
    const char *app_name;
    xiao_ipc_message ipc;
};

typedef struct {
    const char *boot_text;
    const xiao_app *apps;
    xiao_size app_count;
    const xiao_file *files;
    xiao_size file_count;
} xiao_boot_image;

void xiao_start(const xiao_hal *hal, const xiao_boot_image *image);
void xiao_serial_print(xiao_env *env, const char *text);
void xiao_console_print(xiao_env *env, const char *text);
void xiao_console_write(xiao_env *env, const char *data, xiao_size len);
int xiao_input_read(xiao_env *env);
int xiao_exec_app(const char *name);
int xiao_exec_app_args(const char *name, int argc, const char **argv);
int xiao_exec_line(const char *line);
int xiao_argc(xiao_env *env);
const char *xiao_argv(xiao_env *env, int index);
xiao_size xiao_app_count(void);
const char *xiao_app_name(xiao_size index);
xiao_size xiao_file_count(void);
const char *xiao_file_name(xiao_size index);
int xiao_file_read(const char *name, const char **data, xiao_size *size);
void xiao_wait(xiao_env *env, xiao_tick ms);
void xiao_yield(xiao_env *env);

extern const xiao_boot_image xiao_image;

#endif
