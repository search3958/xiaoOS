#include "xiao.h"

#define XIAO_MAX_TASKS 8
#define XIAO_MAX_ARGS 8
#define XIAO_MAX_LINE 128
#define XIAO_FS_MAX_NODES 48
#define XIAO_FS_MAX_PATH 64
#define XIAO_FS_RAM_SIZE 4096

typedef struct {
    const xiao_app *app;
    int active;
} xiao_task;

static xiao_task tasks[XIAO_MAX_TASKS];
static xiao_env current_env;
static const xiao_boot_image *current_image;

typedef struct {
    int used;
    int type;
    char path[XIAO_FS_MAX_PATH];
    const char *ro_data;
    xiao_size ro_size;
    char *data;
    xiao_size size;
    xiao_size capacity;
} xiao_fs_node;

static xiao_fs_node fs_nodes[XIAO_FS_MAX_NODES];
static char fs_ram[XIAO_FS_RAM_SIZE];
static xiao_size fs_ram_used;
static char fs_cwd[XIAO_FS_MAX_PATH];
static int xiao_mode = XIAO_MODE_TEXT;
static xiao_console_sink_fn console_sink;
static void *console_sink_ctx;

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

static int xiao_strcmp(const char *a, const char *b) {
    while (*a && *b && *a == *b) {
        a++;
        b++;
    }
    return (int)(unsigned char)*a - (int)(unsigned char)*b;
}

static void xiao_memcpy(char *dst, const char *src, xiao_size len) {
    xiao_size i;
    for (i = 0; i < len; i++) dst[i] = src[i];
}

static void xiao_strcpy_cap(char *dst, xiao_size cap, const char *src) {
    xiao_size i = 0;
    if (!cap) return;
    while (src && src[i] && i + 1 < cap) {
        dst[i] = src[i];
        i++;
    }
    dst[i] = 0;
}

static int xiao_starts_with_path(const char *path, const char *prefix) {
    xiao_size i = 0;
    while (prefix[i]) {
        if (path[i] != prefix[i]) return 0;
        i++;
    }
    return 1;
}

static void xiao_path_parent(char *dst, xiao_size cap, const char *path) {
    xiao_size len = xiao_strlen(path);
    xiao_size parent_len;
    while (len > 1 && path[len - 1] == '/') len--;
    while (len > 1 && path[len - 1] != '/') len--;
    parent_len = len > 0 ? len - 1 : 0;
    if (parent_len <= 1) {
        xiao_strcpy_cap(dst, cap, "/");
        return;
    }
    if (parent_len >= cap) parent_len = cap - 1;
    xiao_memcpy(dst, path, parent_len);
    dst[parent_len] = 0;
}

static const char *xiao_path_basename(const char *path) {
    const char *base = path;
    while (*path) {
        if (*path == '/' && path[1]) base = path + 1;
        path++;
    }
    return base;
}

static int xiao_path_normalize(const char *input, char *out, xiao_size cap) {
    char temp[XIAO_FS_MAX_PATH];
    xiao_size pos = 0;
    xiao_size i = 0;
    if (!input || !input[0] || !cap) return -1;
    if (input[0] == '/') {
        temp[pos++] = '/';
        temp[pos] = 0;
        input++;
    } else {
        xiao_strcpy_cap(temp, sizeof(temp), fs_cwd[0] ? fs_cwd : "/");
        pos = xiao_strlen(temp);
        if (pos == 0 || temp[pos - 1] != '/') {
            if (pos + 1 >= sizeof(temp)) return -1;
            temp[pos++] = '/';
            temp[pos] = 0;
        }
    }
    while (1) {
        char part[XIAO_FS_MAX_PATH];
        xiao_size n = 0;
        while (input[i] == '/') i++;
        while (input[i] && input[i] != '/') {
            if (n + 1 < sizeof(part)) part[n++] = input[i];
            i++;
        }
        part[n] = 0;
        if (n == 0) break;
        if (xiao_streq(part, ".")) {
        } else if (xiao_streq(part, "..")) {
            while (pos > 1 && temp[pos - 1] == '/') pos--;
            while (pos > 1 && temp[pos - 1] != '/') pos--;
            temp[pos] = 0;
        } else {
            if (pos > 1 && temp[pos - 1] != '/') {
                if (pos + 1 >= sizeof(temp)) return -1;
                temp[pos++] = '/';
            }
            if (pos + n >= sizeof(temp)) return -1;
            xiao_memcpy(temp + pos, part, n);
            pos += n;
            temp[pos] = 0;
        }
        if (!input[i]) break;
    }
    if (pos == 0) {
        temp[pos++] = '/';
        temp[pos] = 0;
    }
    while (pos > 1 && temp[pos - 1] == '/') {
        temp[--pos] = 0;
    }
    xiao_strcpy_cap(out, cap, temp);
    return 0;
}

static xiao_fs_node *xiao_fs_find_abs(const char *path) {
    xiao_size i;
    for (i = 0; i < XIAO_FS_MAX_NODES; i++) {
        if (fs_nodes[i].used && xiao_streq(fs_nodes[i].path, path)) return &fs_nodes[i];
    }
    return 0;
}

static xiao_fs_node *xiao_fs_alloc_node(void) {
    xiao_size i;
    for (i = 0; i < XIAO_FS_MAX_NODES; i++) {
        if (!fs_nodes[i].used) {
            fs_nodes[i].used = 1;
            fs_nodes[i].ro_data = 0;
            fs_nodes[i].ro_size = 0;
            fs_nodes[i].data = 0;
            fs_nodes[i].size = 0;
            fs_nodes[i].capacity = 0;
            return &fs_nodes[i];
        }
    }
    return 0;
}

static int xiao_fs_ensure_dir_abs(const char *path) {
    char parent[XIAO_FS_MAX_PATH];
    xiao_fs_node *node = xiao_fs_find_abs(path);
    if (node) return node->type == XIAO_FS_DIR ? 0 : -1;
    if (!xiao_streq(path, "/")) {
        xiao_path_parent(parent, sizeof(parent), path);
        if (xiao_fs_ensure_dir_abs(parent) != 0) return -1;
    }
    node = xiao_fs_alloc_node();
    if (!node) return -1;
    node->type = XIAO_FS_DIR;
    xiao_strcpy_cap(node->path, sizeof(node->path), path);
    return 0;
}

static int xiao_fs_write_abs(const char *path, const char *data, xiao_size size) {
    char parent[XIAO_FS_MAX_PATH];
    xiao_fs_node *node = xiao_fs_find_abs(path);
    if (!node) {
        xiao_path_parent(parent, sizeof(parent), path);
        if (xiao_fs_ensure_dir_abs(parent) != 0) return -1;
        node = xiao_fs_alloc_node();
        if (!node) return -1;
        xiao_strcpy_cap(node->path, sizeof(node->path), path);
        node->type = XIAO_FS_FILE;
    }
    if (node->type != XIAO_FS_FILE) return -1;
    if (node->capacity < size) {
        if (fs_ram_used + size > sizeof(fs_ram)) return -1;
        node->data = fs_ram + fs_ram_used;
        node->capacity = size;
        fs_ram_used += size;
    }
    if (size) xiao_memcpy(node->data, data, size);
    node->size = size;
    node->ro_data = 0;
    node->ro_size = 0;
    return 0;
}

static int xiao_fs_immediate_child(const char *dir, const char *path) {
    xiao_size len = xiao_strlen(dir);
    const char *rest;
    if (xiao_streq(dir, "/")) {
        if (path[0] != '/' || path[1] == 0) return 0;
        rest = path + 1;
    } else {
        if (xiao_strcmp(path, dir) == 0) return 0;
        if (xiao_strlen(path) <= len || path[len] != '/') return 0;
        if (!xiao_starts_with_path(path, dir)) return 0;
        rest = path + len + 1;
    }
    while (*rest) {
        if (*rest == '/') return 0;
        rest++;
    }
    return 1;
}

static const char *xiao_fs_node_data(xiao_fs_node *node) {
    if (!node || node->type != XIAO_FS_FILE) return 0;
    return node->data ? node->data : node->ro_data;
}

static xiao_size xiao_fs_node_size(xiao_fs_node *node) {
    if (!node || node->type != XIAO_FS_FILE) return 0;
    return node->data ? node->size : node->ro_size;
}

static void xiao_fs_init(const xiao_boot_image *image) {
    xiao_size i;
    fs_ram_used = 0;
    xiao_strcpy_cap(fs_cwd, sizeof(fs_cwd), "/");
    for (i = 0; i < XIAO_FS_MAX_NODES; i++) fs_nodes[i].used = 0;
    xiao_fs_ensure_dir_abs("/");
    if (!image) return;
    for (i = 0; i < image->file_count; i++) {
        char path[XIAO_FS_MAX_PATH];
        char parent[XIAO_FS_MAX_PATH];
        xiao_size path_len;
        xiao_fs_node *node;
        path[0] = '/';
        xiao_strcpy_cap(path + 1, sizeof(path) - 1, image->files[i].name);
        path_len = xiao_strlen(path);
        if (path_len > 1 && path[path_len - 1] == '/') {
            while (path_len > 1 && path[path_len - 1] == '/') {
                path[--path_len] = 0;
            }
            xiao_fs_ensure_dir_abs(path);
            continue;
        }
        xiao_path_parent(parent, sizeof(parent), path);
        if (xiao_fs_ensure_dir_abs(parent) != 0) continue;
        if (xiao_fs_find_abs(path)) continue;
        node = xiao_fs_alloc_node();
        if (!node) continue;
        node->type = XIAO_FS_FILE;
        xiao_strcpy_cap(node->path, sizeof(node->path), path);
        node->ro_data = image->files[i].data;
        node->ro_size = image->files[i].size;
    }
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
    xiao_mode = XIAO_MODE_TEXT;
    console_sink = 0;
    console_sink_ctx = 0;
    xiao_fs_init(image);
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
    if (console_sink && console_sink(console_sink_ctx, data, len)) return;
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

xiao_size xiao_task_slot_count(void) {
    return XIAO_MAX_TASKS;
}

xiao_size xiao_task_active_count(void) {
    xiao_size i;
    xiao_size count = 0;
    for (i = 0; i < XIAO_MAX_TASKS; i++) {
        if (tasks[i].active) count++;
    }
    return count;
}

const char *xiao_task_name(xiao_size index) {
    if (index >= XIAO_MAX_TASKS || !tasks[index].active || !tasks[index].app) return 0;
    return tasks[index].app->name;
}

const char *xiao_current_app(void) {
    return current_env.app_name ? current_env.app_name : "kernel";
}

xiao_size xiao_file_count(void) {
    xiao_size i;
    xiao_size count = 0;
    for (i = 0; i < XIAO_FS_MAX_NODES; i++) {
        if (fs_nodes[i].used && fs_nodes[i].type == XIAO_FS_FILE) count++;
    }
    return count;
}

const char *xiao_file_name(xiao_size index) {
    xiao_size i;
    xiao_size count = 0;
    for (i = 0; i < XIAO_FS_MAX_NODES; i++) {
        if (fs_nodes[i].used && fs_nodes[i].type == XIAO_FS_FILE) {
            if (count == index) return fs_nodes[i].path;
            count++;
        }
    }
    return 0;
}

int xiao_file_read(const char *name, const char **data, xiao_size *size) {
    return xiao_fs_read(name, data, size);
}

const char *xiao_fs_cwd(void) {
    return fs_cwd;
}

int xiao_fs_chdir(const char *path) {
    char abs[XIAO_FS_MAX_PATH];
    xiao_fs_node *node;
    if (xiao_path_normalize(path, abs, sizeof(abs)) != 0) return -1;
    node = xiao_fs_find_abs(abs);
    if (!node || node->type != XIAO_FS_DIR) return -1;
    xiao_strcpy_cap(fs_cwd, sizeof(fs_cwd), abs);
    return 0;
}

int xiao_fs_stat(const char *path, int *type, xiao_size *size) {
    char abs[XIAO_FS_MAX_PATH];
    xiao_fs_node *node;
    if (xiao_path_normalize(path, abs, sizeof(abs)) != 0) return -1;
    node = xiao_fs_find_abs(abs);
    if (!node) return -1;
    if (type) *type = node->type;
    if (size) *size = xiao_fs_node_size(node);
    return 0;
}

int xiao_fs_list(const char *path, xiao_size index, const char **name, int *type, xiao_size *size) {
    xiao_size i;
    xiao_size count = 0;
    char abs[XIAO_FS_MAX_PATH];
    static char display[XIAO_FS_MAX_PATH];
    xiao_fs_node *dir;
    if (xiao_path_normalize(path && path[0] ? path : ".", abs, sizeof(abs)) != 0) return -1;
    dir = xiao_fs_find_abs(abs);
    if (!dir || dir->type != XIAO_FS_DIR) return -1;
    for (i = 0; i < XIAO_FS_MAX_NODES; i++) {
        if (fs_nodes[i].used && xiao_fs_immediate_child(abs, fs_nodes[i].path)) {
            if (count == index) {
                xiao_strcpy_cap(display, sizeof(display), xiao_path_basename(fs_nodes[i].path));
                if (fs_nodes[i].type == XIAO_FS_DIR) {
                    xiao_size len = xiao_strlen(display);
                    if (len + 1 < sizeof(display)) {
                        display[len] = '/';
                        display[len + 1] = 0;
                    }
                }
                if (name) *name = display;
                if (type) *type = fs_nodes[i].type;
                if (size) *size = xiao_fs_node_size(&fs_nodes[i]);
                return 0;
            }
            count++;
        }
    }
    return -1;
}

int xiao_fs_read(const char *path, const char **data, xiao_size *size) {
    char abs[XIAO_FS_MAX_PATH];
    xiao_fs_node *node;
    if (xiao_path_normalize(path, abs, sizeof(abs)) != 0) return -1;
    node = xiao_fs_find_abs(abs);
    if (!node || node->type != XIAO_FS_FILE) return -1;
    if (data) *data = xiao_fs_node_data(node);
    if (size) *size = xiao_fs_node_size(node);
    return 0;
}

int xiao_fs_write(const char *path, const char *data, xiao_size size) {
    char abs[XIAO_FS_MAX_PATH];
    if (xiao_path_normalize(path, abs, sizeof(abs)) != 0) return -1;
    return xiao_fs_write_abs(abs, data, size);
}

int xiao_fs_mkdir(const char *path) {
    char abs[XIAO_FS_MAX_PATH];
    if (xiao_path_normalize(path, abs, sizeof(abs)) != 0) return -1;
    return xiao_fs_ensure_dir_abs(abs);
}

int xiao_fs_remove(const char *path) {
    xiao_size i;
    xiao_size len;
    char abs[XIAO_FS_MAX_PATH];
    if (xiao_path_normalize(path, abs, sizeof(abs)) != 0 || xiao_streq(abs, "/")) return -1;
    if (!xiao_fs_find_abs(abs)) return -1;
    len = xiao_strlen(abs);
    for (i = 0; i < XIAO_FS_MAX_NODES; i++) {
        if (!fs_nodes[i].used) continue;
        if (xiao_streq(fs_nodes[i].path, abs) ||
            (xiao_starts_with_path(fs_nodes[i].path, abs) && fs_nodes[i].path[len] == '/')) {
            fs_nodes[i].used = 0;
        }
    }
    if (xiao_starts_with_path(fs_cwd, abs) && (fs_cwd[len] == 0 || fs_cwd[len] == '/')) {
        xiao_strcpy_cap(fs_cwd, sizeof(fs_cwd), "/");
    }
    return 0;
}

int xiao_fs_rename(const char *old_path, const char *new_path) {
    xiao_size i;
    xiao_size old_len;
    char old_abs[XIAO_FS_MAX_PATH];
    char new_abs[XIAO_FS_MAX_PATH];
    char parent[XIAO_FS_MAX_PATH];
    if (xiao_path_normalize(old_path, old_abs, sizeof(old_abs)) != 0) return -1;
    if (xiao_path_normalize(new_path, new_abs, sizeof(new_abs)) != 0) return -1;
    if (xiao_streq(old_abs, "/") || xiao_fs_find_abs(new_abs) || !xiao_fs_find_abs(old_abs)) return -1;
    old_len = xiao_strlen(old_abs);
    if (xiao_starts_with_path(new_abs, old_abs) && (new_abs[old_len] == 0 || new_abs[old_len] == '/')) return -1;
    xiao_path_parent(parent, sizeof(parent), new_abs);
    if (!xiao_fs_find_abs(parent)) return -1;
    for (i = 0; i < XIAO_FS_MAX_NODES; i++) {
        if (!fs_nodes[i].used) continue;
        if (xiao_streq(fs_nodes[i].path, old_abs)) {
            xiao_strcpy_cap(fs_nodes[i].path, sizeof(fs_nodes[i].path), new_abs);
        } else if (xiao_starts_with_path(fs_nodes[i].path, old_abs) && fs_nodes[i].path[old_len] == '/') {
            char suffix[XIAO_FS_MAX_PATH];
            char merged[XIAO_FS_MAX_PATH];
            xiao_strcpy_cap(suffix, sizeof(suffix), fs_nodes[i].path + old_len);
            xiao_strcpy_cap(merged, sizeof(merged), new_abs);
            if (xiao_strlen(merged) + xiao_strlen(suffix) >= sizeof(merged)) return -1;
            xiao_strcpy_cap(merged + xiao_strlen(merged), sizeof(merged) - xiao_strlen(merged), suffix);
            xiao_strcpy_cap(fs_nodes[i].path, sizeof(fs_nodes[i].path), merged);
        }
    }
    return 0;
}

int xiao_fs_copy(const char *src_path, const char *dst_path) {
    xiao_size i;
    xiao_size src_len;
    char src_abs[XIAO_FS_MAX_PATH];
    char dst_abs[XIAO_FS_MAX_PATH];
    xiao_fs_node *src;
    if (xiao_path_normalize(src_path, src_abs, sizeof(src_abs)) != 0) return -1;
    if (xiao_path_normalize(dst_path, dst_abs, sizeof(dst_abs)) != 0) return -1;
    src = xiao_fs_find_abs(src_abs);
    if (!src || xiao_fs_find_abs(dst_abs)) return -1;
    src_len = xiao_strlen(src_abs);
    if (xiao_starts_with_path(dst_abs, src_abs) && (dst_abs[src_len] == 0 || dst_abs[src_len] == '/')) return -1;

    if (src->type == XIAO_FS_FILE) {
        return xiao_fs_write_abs(dst_abs, xiao_fs_node_data(src), xiao_fs_node_size(src));
    }
    if (src->type != XIAO_FS_DIR || xiao_fs_ensure_dir_abs(dst_abs) != 0) return -1;

    for (i = 0; i < XIAO_FS_MAX_NODES; i++) {
        char merged[XIAO_FS_MAX_PATH];
        const char *suffix;
        if (!fs_nodes[i].used || xiao_streq(fs_nodes[i].path, src_abs)) continue;
        if (!xiao_starts_with_path(fs_nodes[i].path, src_abs) || fs_nodes[i].path[src_len] != '/') continue;
        suffix = fs_nodes[i].path + src_len;
        if (xiao_strlen(dst_abs) + xiao_strlen(suffix) >= sizeof(merged)) return -1;
        xiao_strcpy_cap(merged, sizeof(merged), dst_abs);
        xiao_strcpy_cap(merged + xiao_strlen(merged), sizeof(merged) - xiao_strlen(merged), suffix);
        if (fs_nodes[i].type == XIAO_FS_DIR) {
            if (xiao_fs_ensure_dir_abs(merged) != 0) return -1;
        } else if (fs_nodes[i].type == XIAO_FS_FILE) {
            if (xiao_fs_write_abs(merged, xiao_fs_node_data(&fs_nodes[i]), xiao_fs_node_size(&fs_nodes[i])) != 0) return -1;
        }
    }
    return 0;
}

void xiao_fs_info_read(xiao_fs_info *info) {
    xiao_size i;
    if (!info) return;
    info->nodes_used = 0;
    info->nodes_total = XIAO_FS_MAX_NODES;
    info->files = 0;
    info->dirs = 0;
    info->ram_used = fs_ram_used;
    info->ram_total = sizeof(fs_ram);
    info->rom_used = 0;
    for (i = 0; i < XIAO_FS_MAX_NODES; i++) {
        if (!fs_nodes[i].used) continue;
        info->nodes_used++;
        if (fs_nodes[i].type == XIAO_FS_DIR) {
            info->dirs++;
        } else if (fs_nodes[i].type == XIAO_FS_FILE) {
            info->files++;
            if (!fs_nodes[i].data) info->rom_used += fs_nodes[i].ro_size;
        }
    }
}

void xiao_wait(xiao_env *env, xiao_tick ms) {
    if (env && env->hal && env->hal->wait_ms) env->hal->wait_ms(ms);
}

void xiao_yield(xiao_env *env) {
    if (env && env->hal && env->hal->yield) env->hal->yield();
}

int xiao_video_fill_rgb888(xiao_env *env, unsigned int rgb888) {
    if (env && env->hal && env->hal->video_fill_rgb888) {
        return env->hal->video_fill_rgb888(rgb888);
    }
    return -1;
}

int xiao_video_draw_pixel_rgb888(xiao_env *env, int x, int y, unsigned int rgb888) {
    if (env && env->hal && env->hal->video_draw_pixel_rgb888) {
        return env->hal->video_draw_pixel_rgb888(x, y, rgb888);
    }
    return -1;
}

int xiao_video_fill_rect_rgb888(xiao_env *env, int x, int y, int w, int h, unsigned int rgb888) {
    int iy;
    int ix;
    if (env && env->hal && env->hal->video_fill_rect_rgb888) {
        return env->hal->video_fill_rect_rgb888(x, y, w, h, rgb888);
    }
    if (w <= 0 || h <= 0) return -1;
    for (iy = 0; iy < h; iy++) {
        for (ix = 0; ix < w; ix++) {
            if (xiao_video_draw_pixel_rgb888(env, x + ix, y + iy, rgb888) != 0) return -1;
        }
    }
    return 0;
}

int xiao_video_size(xiao_env *env, int *w, int *h) {
    if (env && env->hal && env->hal->video_size) {
        return env->hal->video_size(w, h);
    }
    return -1;
}

int xiao_video_set_mode(xiao_env *env, int w, int h) {
    if (env && env->hal && env->hal->video_set_mode) {
        return env->hal->video_set_mode(w, h);
    }
    return -1;
}

int xiao_platform(xiao_env *env) {
    if (env && env->hal) return env->hal->platform;
    return XIAO_PLATFORM_UNKNOWN;
}

int xiao_mode_get(void) {
    return xiao_mode;
}

int xiao_mode_set(int mode) {
    if (mode != XIAO_MODE_TEXT && mode != XIAO_MODE_CLI && mode != XIAO_MODE_GUI) return -1;
    xiao_mode = mode;
    return 0;
}

void xiao_console_set_sink(xiao_console_sink_fn sink, void *ctx) {
    console_sink = sink;
    console_sink_ctx = ctx;
}
