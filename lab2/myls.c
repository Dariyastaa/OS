#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <dirent.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <pwd.h>
#include <grp.h>
#include <time.h>
#include <limits.h>

static const char *color_for_mode(mode_t m) {
    if (S_ISDIR(m)) return "\033[34m";
    if (S_ISLNK(m)) return "\033[36m";
    if (m & S_IXUSR) return "\033[32m";
    return "\033[0m";
}

static void build_perm_string(mode_t m, char perms[11]) {
    strcpy(perms, "----------");
    if (S_ISDIR(m)) perms[0] = 'd';
    else if (S_ISLNK(m)) perms[0] = 'l';
    if (m & S_IRUSR) perms[1] = 'r';
    if (m & S_IWUSR) perms[2] = 'w';
    if (m & S_IXUSR) perms[3] = 'x';
    if (m & S_IRGRP) perms[4] = 'r';
    if (m & S_IWGRP) perms[5] = 'w';
    if (m & S_IXGRP) perms[6] = 'x';
    if (m & S_IROTH) perms[7] = 'r';
    if (m & S_IWOTH) perms[8] = 'w';
    if (m & S_IXOTH) perms[9] = 'x';
}

static void get_user_group(uid_t uid, gid_t gid, const char **user, const char **group) {
    static char uid_str[32], gid_str[32];
    struct passwd *pw = getpwuid(uid);
    struct group *gr = getgrgid(gid);
    if (pw) *user = pw->pw_name;
    else { snprintf(uid_str, sizeof(uid_str), "%u", (unsigned)uid); *user = uid_str; }
    if (gr) *group = gr->gr_name;
    else { snprintf(gid_str, sizeof(gid_str), "%u", (unsigned)gid); *group = gid_str; }
}

static void print_long_entry(const char *dir_name, struct dirent *de) {
    struct stat st;
    char path[PATH_MAX], perms[11], time_str[20];
    snprintf(path, sizeof(path), "%s/%s", dir_name, de->d_name);
    if (lstat(path, &st) == -1) { perror("lstat"); return; }

    build_perm_string(st.st_mode, perms);

    const char *user, *group;
    get_user_group(st.st_uid, st.st_gid, &user, &group);

    struct tm *tm_info = localtime(&st.st_mtime);
    if (tm_info) strftime(time_str, sizeof(time_str), "%b %d %H:%M", tm_info);
    else strncpy(time_str, "??? ?? ??:??", sizeof(time_str));

    printf("%s %3lu %-8s %-8s %10lu %s %s%s\033[0m",
           perms,
           (unsigned long)st.st_nlink,
           user,
           group,
           (unsigned long)st.st_size,
           time_str,
           color_for_mode(st.st_mode),
           de->d_name);

    if (S_ISLNK(st.st_mode)) {
        char link_target[PATH_MAX];
        ssize_t len = readlink(path, link_target, sizeof(link_target) - 1);
        if (len != -1) { link_target[len] = '\0'; printf(" -> %s", link_target); }
    }
    putchar('\n');
}

static void print_short_entries(struct dirent **list, int n, int show_all, const char *dir_name) {
    for (int i = 0; i < n; ++i) {
        struct dirent *de = list[i];
        if (!show_all && de->d_name[0] == '.') continue;
        struct stat st;
        char path[PATH_MAX];
        snprintf(path, sizeof(path), "%s/%s", dir_name, de->d_name);
        if (stat(path, &st) == -1) { perror("stat"); continue; }
        printf("%s%s\033[0m  ", color_for_mode(st.st_mode), de->d_name);
    }
    putchar('\n');
}

static long long calc_total_blocks(struct dirent **list, int n, int show_all, const char *dir_name) {
    long long total_blocks = 0;
    for (int i = 0; i < n; ++i) {
        struct dirent *de = list[i];
        if (!show_all && de->d_name[0] == '.') continue;
        struct stat st;
        char path[PATH_MAX];
        snprintf(path, sizeof(path), "%s/%s", dir_name, de->d_name);
        if (lstat(path, &st) == -1) { perror("lstat"); continue; }
        total_blocks += st.st_blocks;   // st_blocks в 512 байтах
    }
    return total_blocks / 2;            // ls показывает в 1K блоках
}

static char *expand_home(const char *path) {
    if (path && path[0] == '~') {
        const char *home = getenv("HOME");
        if (home) {
            size_t h = strlen(home), p = strlen(path);
            char *full = malloc(h + p); // без первой '~'
            if (!full) { perror("malloc"); exit(EXIT_FAILURE); }
            memcpy(full, home, h);
            memcpy(full + h, path + 1, p); // включая '\0'
            return full;
        }
    }
    char *copy = strdup(path ? path : ".");
    if (!copy) { perror("strdup"); exit(EXIT_FAILURE); }
    return copy;
}

int main(int argc, char **argv) {
    int opt, show_all = 0, long_fmt = 0;
    while ((opt = getopt(argc, argv, "la")) != -1) {
        if (opt == 'l') long_fmt = 1;
        else if (opt == 'a') show_all = 1;
        else {
            fprintf(stderr, "Usage: %s [-l] [-a] [directory]\n", argv[0]);
            return EXIT_FAILURE;
        }
    }

    char *dir_name = (optind < argc) ? expand_home(argv[optind]) : expand_home(".");
    DIR *dir = opendir(dir_name);
    if (!dir) {
        fprintf(stderr, "Error opening directory '%s': %s\n", dir_name, strerror(errno));
        free(dir_name);
        return EXIT_FAILURE;
    }

    struct dirent **file_list;
    int n_files = scandir(dir_name, &file_list, NULL, alphasort);
    if (n_files < 0) {
        perror("scandir");
        closedir(dir);
        free(dir_name);
        return EXIT_FAILURE;
    }

    if (long_fmt) {
        long long total = calc_total_blocks(file_list, n_files, show_all, dir_name);
        printf("total %lld\n", total);
        for (int i = 0; i < n_files; ++i) {
            if (!show_all && file_list[i]->d_name[0] == '.') { free(file_list[i]); continue; }
            print_long_entry(dir_name, file_list[i]);
            free(file_list[i]);
        }
    } else {
        print_short_entries(file_list, n_files, show_all, dir_name);
        for (int i = 0; i < n_files; ++i) free(file_list[i]);
    }

    free(file_list);
    closedir(dir);
    free(dir_name);
    return 0;
}