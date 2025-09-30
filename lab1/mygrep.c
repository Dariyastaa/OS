#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int grep_stream(FILE *f, const char *pattern) {
    char *line = NULL;
    size_t cap = 0;
    ssize_t n;
    int matched = 0;

    while ((n = getline(&line, &cap, f)) != -1) {
        if (strstr(line, pattern)) {
            fwrite(line, 1, (size_t)n, stdout);
            matched = 1;
        }
    }
    free(line);
    return matched;
}

int main(int argc, char **argv) {
    if (argc < 2 || argc > 3) {
        fprintf(stderr, "usage: %s pattern [file]\n", argv[0]);
        return 2;
    }

    const char *pattern = argv[1];

    if (argc == 2) {
        return grep_stream(stdin, pattern) ? 0 : 1;
    } else {
        const char *path = argv[2];
        FILE *f = fopen(path, "rb");
        if (!f) { perror("mygrep fopen"); return 2; }
        int ok = grep_stream(f, pattern);
        fclose(f);
        return ok ? 0 : 1;
    }
}