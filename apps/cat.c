#include "xiao.h"

static void usage(xiao_env *env) {
    xiao_console_print(env, "usage: cat FILENAME...\r\n");
}

int xiao_app_entry(xiao_env *env) {
    int i;
    if (xiao_argc(env) < 2) {
        usage(env);
        return 1;
    }

    for (i = 1; i < xiao_argc(env); i++) {
        const char *data = 0;
        xiao_size size = 0;
        const char *name = xiao_argv(env, i);
        
        if (xiao_file_read(name, &data, &size) != 0) {
            xiao_console_print(env, "cat: not found: ");
            xiao_console_print(env, name);
            xiao_console_print(env, "\r\n");
            continue;
        }

        // ファイルが空の場合は改行だけ出力して次へ
        if (size == 0) {
            xiao_console_print(env, "\r\n");
            continue;
        }

        xiao_size pos = 0;
        while (pos < size) {
            xiao_size line_start = pos;
            
            // 改行文字 (\r または \n) が来るまで文字を読み進める
            while (pos < size && data[pos] != '\r' && data[pos] != '\n') {
                pos++;
            }

            // 行の中身（改行以外）をまとめて書き出す
            if (pos > line_start) {
                xiao_console_write(env, &data[line_start], pos - line_start);
            }

            // ターミナル用の正しい改行を明示的に出力
            xiao_console_print(env, "\r\n");

            // ファイル側の改行文字 (\r\n, \n, \r) を安全にスキップ
            if (pos < size && data[pos] == '\r') {
                pos++;
                if (pos < size && data[pos] == '\n') {
                    pos++;
                }
            } else if (pos < size && data[pos] == '\n') {
                pos++;
            }
        }
    }
    return 0;
}