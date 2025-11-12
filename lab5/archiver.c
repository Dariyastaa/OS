#define _GNU_SOURCE
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <time.h>

#define MAGIC "ARX1"
#define MLEN 4
#define BUFSZ 65536

typedef struct {
    uint32_t name_len;      // длина имени
    uint64_t size;          // размер данных
    uint32_t mode;          // права+тип
    uint32_t uid, gid;      // владелец/группа
    int64_t  atime, mtime;  // секунды
} Hdr;

static void help(const char* p){
    dprintf(1,
        "Usage:\n"
        "  %s ARCH -i FILE...\n"
        "  %s ARCH -e FILE...\n"
        "  %s ARCH -s\n"
        "  %s -h\n", p,p,p,p);
}

static int ensure_arch(const char* path){
    int fd = open(path, O_RDWR|O_CLOEXEC);
    if (fd < 0 && errno == ENOENT) {
        fd = open(path, O_RDWR|O_CREAT|O_TRUNC|O_CLOEXEC, 0644);
        if (fd < 0) { perror("open"); exit(1); }
        if (write(fd, MAGIC, MLEN) != MLEN) { perror("write MAGIC"); exit(1); }
        lseek(fd, 0, SEEK_SET);
    } else if (fd < 0) { perror("open"); exit(1); }
    return fd;
}

static int open_ro(const char* path){
    int fd = open(path, O_RDONLY|O_CLOEXEC);
    if (fd < 0) { perror("open"); exit(1); }
    return fd;
}

static int magic(int fd){
    char m[MLEN];
    if (read(fd, m, MLEN) != MLEN) return 0;
    return memcmp(m, MAGIC, MLEN)==0;
}

static int read_entry(int fd, Hdr* h, char** name, off_t* data_off){
    if (read(fd, h, sizeof(*h)) != (ssize_t)sizeof(*h)) return 0;
    *name = (char*)malloc(h->name_len + 1);
    if (!*name) exit(1);
    if (read(fd, *name, h->name_len) != (ssize_t)h->name_len) { free(*name); return 0; }
    (*name)[h->name_len] = 0;
    *data_off = lseek(fd, 0, SEEK_CUR);
    lseek(fd, (off_t)h->size, SEEK_CUR);
    return 1;
}

static void copy_n(int in, int out, uint64_t n){
    static char buf[BUFSZ];
    while (n) {
        size_t k = n > BUFSZ ? BUFSZ : (size_t)n;
        ssize_t r = read(in, buf, k);
        if (r <= 0) break;
        write(out, buf, r);
        n -= (size_t)r;
    }
}

static int in_list(const char* name, int n, char** list){
    for (int i=0;i<n;i++) if (!strcmp(name, list[i])) return 1;
    return 0;
}

static void repack_excluding(const char* arch, int ex_n, char** ex_list){
    int in = open(arch, O_RDONLY|O_CLOEXEC);
    if (in < 0 && errno == ENOENT) {
        int out = open("_tmp.new", O_WRONLY|O_CREAT|O_TRUNC|O_CLOEXEC, 0644);
        write(out, MAGIC, MLEN);
        close(out);
        rename("_tmp.new", arch);
        return;
    } else if (in < 0) { perror("open repack"); exit(1); }

    if (!magic(in)) {
        close(in);
        int out = open("_tmp.new", O_WRONLY|O_CREAT|O_TRUNC|O_CLOEXEC, 0644);
        write(out, MAGIC, MLEN);
        close(out);
        rename("_tmp.new", arch);
        return;
    }

    int out = open("_tmp.new", O_WRONLY|O_CREAT|O_TRUNC|O_CLOEXEC, 0644);
    write(out, MAGIC, MLEN);

    Hdr h; char* name; off_t data_off;
    while (read_entry(in, &h, &name, &data_off)) {
        if (!in_list(name, ex_n, ex_list)) {
            write(out, &h, sizeof(h));
            write(out, name, h.name_len);
            lseek(in, data_off, SEEK_SET);
            copy_n(in, out, h.size);
            lseek(in, data_off + (off_t)h.size, SEEK_SET);
        }
        free(name);
    }
    close(in); fsync(out); close(out);
    if (rename("_tmp.new", arch) < 0) { perror("rename"); exit(1); }
}

static void cmd_insert(const char* arch, int n, char** files){
    close(ensure_arch(arch));
    repack_excluding(arch, n, files);

    int a = ensure_arch(arch);
    lseek(a, 0, SEEK_END);

    for (int i=0;i<n;i++){
        const char* p = files[i];
        struct stat st;
        if (stat(p, &st)!=0 || !S_ISREG(st.st_mode)) { dprintf(2,"skip %s\n",p); continue; }
        int in = open(p, O_RDONLY|O_CLOEXEC);
        if (in < 0) { dprintf(2,"skip %s\n",p); continue; }

        Hdr h = {0};
        h.name_len = (uint32_t)strlen(p);
        h.size     = (uint64_t)st.st_size;
        h.mode     = (uint32_t)st.st_mode;
        h.uid      = st.st_uid; h.gid = st.st_gid;
        h.atime    = st.st_atim.tv_sec;
        h.mtime    = st.st_mtim.tv_sec;

        write(a, &h, sizeof(h));
        write(a, p, h.name_len);
        copy_n(in, a, h.size);
        close(in);
        dprintf(1,"added: %s\n", p);
    }
    fsync(a); close(a);
}

static void cmd_extract(const char* arch, int n, char** files){
    int a = open_ro(arch);
    if (!magic(a)) { dprintf(2,"bad archive\n"); close(a); return; }

    Hdr h; char* name; off_t data_off;
    int* found = (int*)calloc(n, sizeof(int));

    while (read_entry(a, &h, &name, &data_off)) {
        if (!in_list(name, n, files)) { free(name); continue; }

        int out = open(name, O_WRONLY|O_CREAT|O_TRUNC|O_CLOEXEC, 0600);
        if (out < 0) { dprintf(2,"extract %s: %s\n", name, strerror(errno)); free(name); continue; }

        lseek(a, data_off, SEEK_SET);
        copy_n(a, out, h.size);

        fchmod(out, h.mode & 07777);
        if (fchown(out, h.uid, h.gid) < 0 && errno!=EPERM) {}
        struct timespec ts[2] = { {h.atime,0}, {h.mtime,0} };
        futimens(out, ts);
        close(out);

        for (int i=0;i<n;i++) if (!strcmp(name, files[i])) found[i]=1;
        lseek(a, data_off + (off_t)h.size, SEEK_SET);

        dprintf(1,"extracted: %s\n", name);
        free(name);
    }
    close(a);

    int ex_n=0; for (int i=0;i<n;i++) if (found[i]) ex_n++;
    if (ex_n) {
        char** ex = (char**)malloc(ex_n*sizeof(char*));
        int k=0; for (int i=0;i<n;i++) if (found[i]) ex[k++]=files[i];
        repack_excluding(arch, ex_n, ex);
        free(ex);
    }
    for (int i=0;i<n;i++) if (!found[i]) dprintf(2,"not found: %s\n", files[i]);
    free(found);
}

static void cmd_stat(const char* arch){
    int a = open_ro(arch);
    if (!magic(a)) { dprintf(2,"bad archive\n"); close(a); return; }

    Hdr h; char* name; off_t data_off;
    uint64_t total=0; int cnt=0;

    dprintf(1,"Archive: %s\n", arch);
    while (read_entry(a, &h, &name, &data_off)) {
        char ab[64], mb[64];
        time_t as=(time_t)h.atime, ms=(time_t)h.mtime;
        strftime(ab,sizeof(ab),"%F %T", localtime(&as));
        strftime(mb,sizeof(mb),"%F %T", localtime(&ms));
        dprintf(1,
            "%s\n  size: %llu\n  mode: %o\n  uid:gid=%u:%u\n  atime:%s\n  mtime:%s\n",
            name, (unsigned long long)h.size, (unsigned)(h.mode&07777),
            h.uid, h.gid, ab, mb);
        total += h.size; cnt++; free(name);
    }
    dprintf(1,"-- files: %d, total: %llu bytes\n", cnt, (unsigned long long)total);
    close(a);
}

int main(int argc, char** argv){
    if (argc<2){ help(argv[0]); return 1; }
    if (!strcmp(argv[1],"-h")||!strcmp(argv[1],"--help")){ help(argv[0]); return 0; }
    if (argc<3){ help(argv[0]); return 1; }

    const char* arch = argv[1];
    const char* cmd  = argv[2];

    if (!strcmp(cmd,"-i")||!strcmp(cmd,"--input")){
        if (argc<4){ dprintf(2,"no files\n"); return 1; }
        cmd_insert(arch, argc-3, &argv[3]);
    } else if (!strcmp(cmd,"-e")||!strcmp(cmd,"--extract")){
        if (argc<4){ dprintf(2,"no files\n"); return 1; }
        cmd_extract(arch, argc-3, &argv[3]);
    } else if (!strcmp(cmd,"-s")||!strcmp(cmd,"--stat")){
        cmd_stat(arch);
    } else {
        help(argv[0]); return 1;
    }
    return 0;
}