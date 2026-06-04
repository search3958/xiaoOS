#include "xiao.h"

// 文字列比較用の補助関数
static int xiao_streq_local(const char *a, const char *b) {
    while (*a && *b && *a == *b) {
        a++;
        b++;
    }
    return *a == 0 && *b == 0;
}

// 文字列を整数(int)に変換する補助関数 (atoiの代用)
static int parse_int(const char *str) {
    int res = 0;
    while (*str >= '0' && *str <= '9') {
        res = res * 10 + (*str - '0');
        str++;
    }
    return res;
}

static void usage(xiao_env *env) {
    xiao_console_print(env, "usage: head [-n lines] [-q] FILENAME...\r\n");
}

int xiao_app_entry(xiao_env *env) {
    int argc = xiao_argc(env);
    if (argc < 2) {
        usage(env);
        return 1;
    }

    int num_lines = 10; // デフォルトの表示行数は10行
    int quiet = 0;      // デフォルトはヘッダー表示あり (-qで1にする)
    
    // ファイル引数のインデックスを保存する配列 (最大32ファイルまで対応)
    int file_indices[32];
    int file_count = 0;
    
    // 引数の解析
    for (int i = 1; i < argc; i++) {
        const char *arg = xiao_argv(env, i);
        
        if (xiao_streq_local(arg, "-n")) {
            // -n の次は行数の数字が来ると想定
            if (i + 1 < argc) {
                i++;
                num_lines = parse_int(xiao_argv(env, i));
            } else {
                xiao_console_print(env, "head: option requires an argument -- n\r\n");
                return 1;
            }
        } else if (xiao_streq_local(arg, "-q")) {
            quiet = 1;
        } else {
            // オプションではない引数はファイル名として処理
            if (file_count < 32) {
                file_indices[file_count++] = i;
            }
        }
    }

    if (file_count == 0) {
        usage(env);
        return 1;
    }

    int first_file = 1;

    // 指定されたファイルを順に処理
    for (int k = 0; k < file_count; k++) {
        const char *name = xiao_argv(env, file_indices[k]);
        const char *data = 0;
        xiao_size size = 0;

        if (xiao_file_read(name, &data, &size) != 0) {
            xiao_console_print(env, "head: not found: ");
            xiao_console_print(env, name);
            xiao_console_print(env, "\r\n");
            continue;
        }

        // 標準的な head と同様に、複数ファイル指定時はファイル名をヘッダー出力する
        // ただし -q が指定されている場合は出力しない
        if (file_count > 1 && !quiet) {
            if (!first_file) {
                xiao_console_print(env, "\r\n"); // 複数ファイル間の区切り用空行
            }
            xiao_console_print(env, "==> ");
            xiao_console_print(env, name);
            xiao_console_print(env, " <==\r\n");
        }
        first_file = 0;

        // 空ファイルの場合は次へ
        if (size == 0) continue;

        xiao_size pos = 0;
        int current_line = 0;

        // 指定された行数 (num_lines) に到達するまで、またはファイル終端まで読み込む
        while (pos < size && current_line < num_lines) {
            xiao_size line_start = pos;
            
            // 改行までポインタを進める
            while (pos < size && data[pos] != '\r' && data[pos] != '\n') {
                pos++;
            }

            // 行の印字
            if (pos > line_start) {
                xiao_console_write(env, &data[line_start], pos - line_start);
            }

            // ターミナル用の正しい改行(\r\n)を強制
            xiao_console_print(env, "\r\n");
            current_line++;

            // ファイル側の改行コードを安全に読み飛ばす (\r\n, \n, \r に対応)
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