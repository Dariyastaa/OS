#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

static void cat_stream(FILE *f, bool number_all, bool number_nonblank, bool show_ends, int *ln_counter) {
    char *line = NULL;
    size_t cap = 0;
    ssize_t n;

    while ((n = getline(&line, &cap, f)) != -1) {
        int print_num = 0;

        if (number_nonblank) {
            int is_blank = (n == 1 && line[0] == '\n');
            if (!is_blank) print_num = 1;
        } else if (number_all) {
            print_num = 1;
        }

        if (print_num) {
            printf("%6d\t", (*ln_counter)++);
        }

        if (show_ends) {
            if (n > 0 && line[n - 1] == '\n') {
                line[n - 1] = '\0';
                fputs(line, stdout);
                fputc('$', stdout);
                fputc('\n', stdout);
            } else {
                fputs(line, stdout);
                fputc('$', stdout);
            }
        } else {
            fwrite(line, 1, (size_t)n, stdout);
        }
    }
    free(line);
}

int main(int argc, char **argv) {
    bool flag_n = false; // -n: нумеровать все
    bool flag_b = false; // -b: нумеровать непустые
    bool flag_E = false; // -E: показывать $
    int i = 1;

    for (; i < argc; ++i) {
        if (argv[i][0] != '-' || strcmp(argv[i], "-") == 0) break;
        if (strcmp(argv[i], "--") == 0) { i++; break; }
        const char *p = argv[i] + 1;
        if (*p == '\0') break;
        for (; *p; ++p) {
            switch (*p) {
                case 'n': flag_n = true; break;
                case 'b': flag_b = true; break;
                case 'E': flag_E = true; break;
                default:
                    fprintf(stderr, "mycat: unknown option -%c\n", *p);
                    return 1;
            }
        }
    }

    bool number_nonblank = flag_b;
    bool number_all = flag_b ? false : flag_n;

    int line_counter = 1;
    int exit_code = 0;

    if (i >= argc) {
        cat_stream(stdin, number_all, number_nonblank, flag_E, &line_counter);
        return 0;
    }

    for (; i < argc; ++i) {
        const char *path = argv[i];
        if (strcmp(path, "-") == 0) {
            cat_stream(stdin, number_all, number_nonblank, flag_E, &line_counter);
            continue;
        }
        FILE *f = fopen(path, "rb");
        if (!f) {
            perror("mycat fopen");
            exit_code = 1;
            continue;
        }
        cat_stream(f, number_all, number_nonblank, flag_E, &line_counter);
        fclose(f);
    }
    return exit_code;
}