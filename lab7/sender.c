#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/types.h>
#include <errno.h>
#include <signal.h>

#define SHM_SIZE 1024 

int shm_id;
char *shared_mem;

void cleanup(int sig) {
    if (shared_mem != (char *)-1)
        shmdt(shared_mem);
    if (shm_id != -1)
        shmctl(shm_id, IPC_RMID, NULL);
    printf("\nShared memory удалена, sender завершён\n");
    exit(0);
}

int main() {
    signal(SIGINT, cleanup);
    signal(SIGTERM, cleanup);

    key_t shm_key = ftok("shmfile", 65);

    shm_id = shmget(shm_key, SHM_SIZE, IPC_CREAT | IPC_EXCL | 0666);
    if (shm_id == -1) {
        if (errno == EEXIST) {
            printf("Sender уже запущен\n");
        } else {
            perror("shmget");
        }
        exit(1);
    }

    shared_mem = (char*) shmat(shm_id, NULL, 0);
    if (shared_mem == (char *)(-1)) {
        perror("shmat");
        shmctl(shm_id, IPC_RMID, NULL);
        exit(1);
    }

    pid_t sender_pid = getpid();
    time_t current_time;
    struct tm *time_info;
    char time_str[80];

    while(1) {
        time(&current_time);
        time_info = localtime(&current_time);
        strftime(time_str, sizeof(time_str), "%Y-%m-%d %H:%M:%S", time_info);
        sprintf(shared_mem, "SenderTime: %s | SenderPID: %d\n", time_str, sender_pid);
        sleep(2);
    }

    cleanup(0);
    return 0;
}