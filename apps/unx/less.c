#include "xiao.h"

#define PAGE_LINES 23

static void usage(xiao_env *env) {
    xiao_console_print(env, "usage: less FILENAME\r\n");
}

static int wait_for_key(xiao_env *env) {
    while (1) {
        int ch = xiao_input_read(env);
        if (ch >= 0) {
            return ch;
        }
        xiao_wait(env, 10);
    }
}

int xiao_app_entry(xiao_env *env) {
    if (xiao_argc(env) < 2) {
        usage(env);
        return 1;
    }

    const char *name = xiao_argv(env, 1);
    const char *data = 0;
    xiao_size size = 0;

    if (xiao_file_read(name, &data, &size) != 0) {
        xiao_console_print(env, "less: not found: ");
        xiao_console_print(env, name);
        xiao_console_print(env, "\r\n");
        return 1;
    }

    int lines_to_show = PAGE_LINES;
    xiao_size i = 0;

    while (i < size) {
        if (lines_to_show <= 0) {
            xiao_console_print(env, "--More--");
            
            int c;
            while (1) {
                c = wait_for_key(env);
                
                if (c == 'q' || c == 'Q') {
                    xiao_console_print(env, "\r        \r");
                    return 0;
                } else if (c == ' ') {
                    lines_to_show = PAGE_LINES;
                    break;
                } else if (c == '\r' || c == '\n') {
                    lines_to_show = 1;
                    break;
                }
            }
            xiao_console_print(env, "\r        \r");
        }

        // 改行文字 (\r または \n) が来るまでインデックスを進める
        xiao_size line_start = i;
        while (i < size && data[i] != '\r' && data[i] != '\n') {
            i++;
        }

        // 行の中身だけをコンソールに書き出す（改行文字は含まれない）
        if (i > line_start) {
            xiao_console_write(env, &data[line_start], i - line_start);
        }

        // ターミナルに向けて、環境依存しない綺麗な改行 (\r\n) を明示的に出力
        xiao_console_print(env, "\r\n");
        lines_to_show--;

        // ファイル側の改行文字をスキップする
        // Windows(\r\n), Mac(\r), Unix(\n) すべてに対応
        if (i < size && data[i] == '\r') {
            i++;
            if (i < size && data[i] == '\n') {
                i++; // \r\n の場合はさらに進める
            }
        } else if (i < size && data[i] == '\n') {
            i++;
        }
    }

    return 0;
}