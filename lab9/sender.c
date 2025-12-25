#define _XOPEN_SOURCE 700

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <signal.h>
#include <errno.h>

#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/sem.h>

#define SHM_SIZE 256
#define FTOK_PATH "."
#define FTOK_PROJ_ID  'S'

static int shm_id = -1;
static int sem_id = -1;
static char *shm_ptr = (char*)-1;
static volatile sig_atomic_t running = 1;

union semun {
    int val;
    struct semid_ds *buf;
    unsigned short *array;
#if defined(__linux__)
    struct seminfo *__buf;
#endif
};

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

static void cleanup(void) {
    if (shm_ptr != (char*)-1) shmdt(shm_ptr);
    if (shm_id != -1) shmctl(shm_id, IPC_RMID, NULL);
    if (sem_id != -1) semctl(sem_id, 0, IPC_RMID);
}

static void on_sigint(int sig) {
    (void)sig;
    running = 0;
}

static void format_time(char *out, size_t out_sz) {
    time_t t = time(NULL);
    struct tm tmv;
    localtime_r(&t, &tmv);
    strftime(out, out_sz, "%H:%M:%S", &tmv);
}

int main(void) {
    signal(SIGINT, on_sigint);

    key_t key = ftok(FTOK_PATH, FTOK_PROJ_ID);
    if (key == -1) {
        perror("ftok");
        return 1;
    }

    shm_id = shmget(key, SHM_SIZE, IPC_CREAT | 0666);
    if (shm_id == -1) {
        perror("shmget");
        return 1;
    }

    shm_ptr = (char*)shmat(shm_id, NULL, 0);
    if (shm_ptr == (char*)-1) {
        perror("shmat");
        cleanup();
        return 1;
    }

    sem_id = semget(key, 1, IPC_CREAT | 0666);
    if (sem_id == -1) {
        perror("semget");
        cleanup();
        return 1;
    }

    union semun arg;
    arg.val = 1;
    if (semctl(sem_id, 0, SETVAL, arg) == -1) {
        perror("semctl SETVAL");
        cleanup();
        return 1;
    }

    sem_P(sem_id);
    snprintf(shm_ptr, SHM_SIZE, "init by sender pid=%d", getpid());
    sem_V(sem_id);

    while (running) {
        char tbuf[32];
        format_time(tbuf, sizeof(tbuf));

        sem_P(sem_id);
        snprintf(shm_ptr, SHM_SIZE, "send_time=%s; sender_pid=%d", tbuf, getpid());
        sem_V(sem_id);

        sleep(3);
    }

    cleanup();
    return 0;
}