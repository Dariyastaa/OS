#define _XOPEN_SOURCE 700

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <errno.h>

#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/sem.h>

#define SHM_SIZE 256
#define FTOK_PATH "."
#define FTOK_PROJ_ID  'S'

static void sem_P(int semid) {
    struct sembuf op = {0, -1, 0};
    while (semop(semid, &op, 1) == -1) {
        if (errno == EINTR) continue;
        perror("semop P");
        exit(1);
    }
}

static void sem_V(int semid) {
    struct sembuf op = {0, +1, 0};
    while (semop(semid, &op, 1) == -1) {
        if (errno == EINTR) continue;
        perror("semop V");
        exit(1);
    }
}

static void format_time(char *out, size_t out_sz) {
    time_t t = time(NULL);
    struct tm tmv;
    localtime_r(&t, &tmv);
    strftime(out, out_sz, "%H:%M:%S", &tmv);
}

int main(void) {
    key_t key = ftok(FTOK_PATH, FTOK_PROJ_ID);
    if (key == -1) {
        perror("ftok");
        return 1;
    }

    int shm_id, sem_id;
    char *shm_ptr;

    while (1) {
        shm_id = shmget(key, SHM_SIZE, 0666);
        sem_id = semget(key, 1, 0666);

        if (shm_id != -1 && sem_id != -1) break;

        fprintf(stderr, "receiver: waiting for sender...\n");
        sleep(1);
    }

    shm_ptr = (char*)shmat(shm_id, NULL, 0);
    if (shm_ptr == (char*)-1) {
        perror("shmat");
        return 1;
    }

    while (1) {
        char now[32];
        format_time(now, sizeof(now));

        sem_P(sem_id);
        printf("recv_time=%s; recv_pid=%d; msg=\"%s\"\n", now, getpid(), shm_ptr);
        fflush(stdout);
        sem_V(sem_id);

        sleep(1);
    }

    shmdt(shm_ptr);
    return 0;
}