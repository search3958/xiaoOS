#include "xiao.h"

#define XIAO_MAX_TASKS 8

typedef struct {
    const xiao_app *app;
    int active;
} xiao_task;

static xiao_task tasks[XIAO_MAX_TASKS];
static xiao_env current_env;
static const xiao_boot_image *current_image;

static xiao_size xiao_strlen(const char *s) {
    xiao_size n = 0;
    while (s && s[n]) n++;
    return n;
}

static int xiao_streq(const char *a, const char *b) {
    while (*a && *b && *a == *b) {
        a++;
        b++;
    }
    return *a == 0 && *b == 0;
}

static void xiao_copy_word(char *dst, xiao_size cap, const char **src) {
    xiao_size n = 0;
    while (**src == ' ' || **src == '\t') (*src)++;
    while (**src && **src != ' ' && **src != '\t' && **src != '\r' && **src != '\n') {
        if (n + 1 < cap) dst[n++] = **src;
        (*src)++;
    }
    dst[n] = 0;
}

static unsigned long xiao_parse_uint(const char *s) {
    unsigned long v = 0;
    while (*s >= '0' && *s <= '9') {
        v = v * 10 + (unsigned long)(*s - '0');
        s++;
    }
    return v;
}

static const xiao_app *xiao_find_app(const xiao_boot_image *image, const char *name) {
    xiao_size i;
    for (i = 0; i < image->app_count; i++) {
        if (xiao_streq(image->apps[i].name, name)) return &image->apps[i];
    }
    return 0;
}

static void xiao_run_app(const xiao_app *app) {
    if (app && app->main) app->main(&current_env);
}

static void xiao_spawn(const xiao_app *app) {
    xiao_size i;
    for (i = 0; i < XIAO_MAX_TASKS; i++) {
        if (!tasks[i].active) {
            tasks[i].app = app;
            tasks[i].active = 1;
            return;
        }
    }
}

static void xiao_drain_tasks(void) {
    xiao_size i;
    for (i = 0; i < XIAO_MAX_TASKS; i++) {
        if (tasks[i].active) {
            const xiao_app *app = tasks[i].app;
            tasks[i].active = 0;
            xiao_run_app(app);
        }
    }
}

static void xiao_skip_line(const char **p) {
    while (**p && **p != '\n') (*p)++;
    if (**p == '\n') (*p)++;
}

static void xiao_run_boot_text(const xiao_boot_image *image) {
    const char *p = image->boot_text;
    char cmd[16];
    char arg[48];
    while (p && *p) {
        while (*p == ' ' || *p == '\t' || *p == '\r' || *p == '\n') p++;
        if (*p == 0) break;
        if (*p == '#') {
            xiao_skip_line(&p);
            continue;
        }
        xiao_copy_word(cmd, sizeof(cmd), &p);
        xiao_copy_word(arg, sizeof(arg), &p);
        if (xiao_streq(cmd, "exec")) {
            xiao_run_app(xiao_find_app(image, arg));
        } else if (xiao_streq(cmd, "spawn")) {
            xiao_spawn(xiao_find_app(image, arg));
        } else if (xiao_streq(cmd, "wait")) {
            if (xiao_streq(arg, "forever")) {
                while (1) {
                    xiao_drain_tasks();
                    xiao_wait(&current_env, 100);
                }
            }
            xiao_wait(&current_env, xiao_parse_uint(arg));
        }
        xiao_drain_tasks();
        xiao_skip_line(&p);
    }
}

void xiao_start(const xiao_hal *hal, const xiao_boot_image *image) {
    xiao_size i;
    current_env.hal = hal;
    current_image = image;
    for (i = 0; i < XIAO_MAX_TASKS; i++) tasks[i].active = 0;
    if (image) xiao_run_boot_text(image);
    while (1) xiao_wait(&current_env, 1000);
}

void xiao_serial_print(xiao_env *env, const char *text) {
    if (env && env->hal && env->hal->serial_write) {
        env->hal->serial_write(text, xiao_strlen(text));
    }
}

void xiao_console_print(xiao_env *env, const char *text) {
    if (env && env->hal && env->hal->console_write) {
        env->hal->console_write(text, xiao_strlen(text));
    }
}

int xiao_input_read(xiao_env *env) {
    if (env && env->hal && env->hal->input_read) return env->hal->input_read();
    return -1;
}

int xiao_exec_app(const char *name) {
    const xiao_app *app;
    if (!current_image || !name) return -1;
    app = xiao_find_app(current_image, name);
    if (!app) return -1;
    xiao_run_app(app);
    return 0;
}

void xiao_wait(xiao_env *env, xiao_tick ms) {
    if (env && env->hal && env->hal->wait_ms) env->hal->wait_ms(ms);
}

void xiao_yield(xiao_env *env) {
    if (env && env->hal && env->hal->yield) env->hal->yield();
}
