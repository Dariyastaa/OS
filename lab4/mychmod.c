#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/stat.h>

static mode_t parse_num(const char *s) {
    char *e;
    long v = strtol(s, &e, 8);
    if (*e || v < 0 || v > 0777) {
        fprintf(stderr, "Ошибка: некорректный числовой режим\n");
        exit(1);
    }
    return (mode_t)v;
}

static mode_t parse_sym(const char *s, mode_t old) {
    const char *p = s;
    mode_t who = 0, perm = 0, all = S_IRWXU | S_IRWXG | S_IRWXO;

    while (*p == 'u' || *p == 'g' || *p == 'o' || *p == 'a') {
        if (*p == 'u') who |= S_IRWXU;
        else if (*p == 'g') who |= S_IRWXG;
        else if (*p == 'o') who |= S_IRWXO;
        else if (*p == 'a') who |= all;
        p++;
    }
    if (!who) who = all;

    char op = *p++;
    if (op != '+' && op != '-') {
        fprintf(stderr, "Ошибка: нет оператора (- или +)\n");
        exit(1);
    }

    for (; *p; p++) {
        if (*p == 'r') perm |= S_IRUSR | S_IRGRP | S_IROTH;
        else if (*p == 'w') perm |= S_IWUSR | S_IWGRP | S_IWOTH;
        else if (*p == 'x') perm |= S_IXUSR | S_IXGRP | S_IXOTH;
        else {
            fprintf(stderr, "Ошибка: неверный символ\n");
            exit(1);
        }
    }
    return (op == '+') ? (old | (who & perm)) : (old & ~(who & perm));
}

int main(int argc, char **argv) {
    if (argc != 3) {
        return 1;
    }

    const char *mode = argv[1];
    const char *path = argv[2];

    struct stat st;
    if (stat(path, &st) < 0) {
        perror("stat");
        return 1;
    }

    mode_t new_mode = isdigit((unsigned char)mode[0])
        ? parse_num(mode)
        : parse_sym(mode, st.st_mode);

    if (chmod(path, new_mode) < 0) {
        perror("chmod");
        return 1;
    }
    return 0;
}