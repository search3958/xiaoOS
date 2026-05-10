#include "xiao.h"

#define XIAO_MAX_TASKS 8
#define XIAO_MAX_ARGS 8
#define XIAO_MAX_LINE 128

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

static void xiao_run_app_args(const xiao_app *app, int argc, const char **argv) {
    const char *saved_app_name = current_env.app_name;
    xiao_ipc_message saved_ipc = current_env.ipc;
    if (!app || !app->main) return;
    current_env.app_name = app->name;
    current_env.ipc.from = saved_app_name ? saved_app_name : "kernel";
    current_env.ipc.argc = argc;
    current_env.ipc.argv = argv;
    app->main(&current_env);
    current_env.app_name = saved_app_name;
    current_env.ipc = saved_ipc;
}

static void xiao_run_app(const xiao_app *app) {
    const char *argv[1];
    if (!app) return;
    argv[0] = app->name;
    xiao_run_app_args(app, 1, argv);
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

static void xiao_copy_line(char *dst, xiao_size cap, const char **src) {
    xiao_size n = 0;
    while (**src == ' ' || **src == '\t') (*src)++;
    while (**src && **src != '\r' && **src != '\n') {
        if (n + 1 < cap) dst[n++] = **src;
        (*src)++;
    }
    dst[n] = 0;
}

static void xiao_run_boot_text(const xiao_boot_image *image) {
    const char *p = image->boot_text;
    char cmd[16];
    char arg[48];
    char line[XIAO_MAX_LINE];
    while (p && *p) {
        while (*p == ' ' || *p == '\t' || *p == '\r' || *p == '\n') p++;
        if (*p == 0) break;
        if (*p == '#') {
            xiao_skip_line(&p);
            continue;
        }
        xiao_copy_word(cmd, sizeof(cmd), &p);
        if (xiao_streq(cmd, "exec")) {
            xiao_copy_line(line, sizeof(line), &p);
            xiao_exec_line(line);
        } else if (xiao_streq(cmd, "spawn")) {
            xiao_copy_word(arg, sizeof(arg), &p);
            xiao_spawn(xiao_find_app(image, arg));
        } else if (xiao_streq(cmd, "wait")) {
            xiao_copy_word(arg, sizeof(arg), &p);
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
    current_env.app_name = 0;
    current_env.ipc.from = "kernel";
    current_env.ipc.argc = 0;
    current_env.ipc.argv = 0;
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
    xiao_console_write(env, text, xiao_strlen(text));
}

void xiao_console_write(xiao_env *env, const char *data, xiao_size len) {
    if (env && env->hal && env->hal->console_write) {
        env->hal->console_write(data, len);
    }
}

int xiao_input_read(xiao_env *env) {
    if (env && env->hal && env->hal->input_read) return env->hal->input_read();
    return -1;
}

int xiao_exec_app(const char *name) {
    const char *argv[1];
    if (!name) return -1;
    argv[0] = name;
    return xiao_exec_app_args(name, 1, argv);
}

int xiao_exec_app_args(const char *name, int argc, const char **argv) {
    const xiao_app *app;
    if (!current_image || !name) return -1;
    app = xiao_find_app(current_image, name);
    if (!app) return -1;
    xiao_run_app_args(app, argc, argv);
    return 0;
}

int xiao_exec_line(const char *line) {
    static char copy[XIAO_MAX_LINE];
    static const char *argv[XIAO_MAX_ARGS];
    xiao_size i = 0;
    int argc = 0;
    if (!line) return -1;
    while (line[i] && i + 1 < sizeof(copy)) {
        copy[i] = line[i];
        i++;
    }
    copy[i] = 0;
    i = 0;
    while (copy[i] && argc < XIAO_MAX_ARGS) {
        while (copy[i] == ' ' || copy[i] == '\t') i++;
        if (!copy[i]) break;
        argv[argc++] = &copy[i];
        while (copy[i] && copy[i] != ' ' && copy[i] != '\t') i++;
        if (copy[i]) copy[i++] = 0;
    }
    if (argc == 0) return 0;
    return xiao_exec_app_args(argv[0], argc, argv);
}

int xiao_argc(xiao_env *env) {
    return env ? env->ipc.argc : 0;
}

const char *xiao_argv(xiao_env *env, int index) {
    if (!env || index < 0 || index >= env->ipc.argc || !env->ipc.argv) return 0;
    return env->ipc.argv[index];
}

xiao_size xiao_app_count(void) {
    return current_image ? current_image->app_count : 0;
}

const char *xiao_app_name(xiao_size index) {
    if (!current_image || index >= current_image->app_count) return 0;
    return current_image->apps[index].name;
}

xiao_size xiao_file_count(void) {
    return current_image ? current_image->file_count : 0;
}

const char *xiao_file_name(xiao_size index) {
    if (!current_image || index >= current_image->file_count) return 0;
    return current_image->files[index].name;
}

int xiao_file_read(const char *name, const char **data, xiao_size *size) {
    xiao_size i;
    if (!current_image || !name) return -1;
    for (i = 0; i < current_image->file_count; i++) {
        if (xiao_streq(current_image->files[i].name, name)) {
            if (data) *data = current_image->files[i].data;
            if (size) *size = current_image->files[i].size;
            return 0;
        }
    }
    return -1;
}

void xiao_wait(xiao_env *env, xiao_tick ms) {
    if (env && env->hal && env->hal->wait_ms) env->hal->wait_ms(ms);
}

void xiao_yield(xiao_env *env) {
    if (env && env->hal && env->hal->yield) env->hal->yield();
}
