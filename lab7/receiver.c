#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/types.h>

#define SHM_SIZE 1024

int main() {
    key_t shm_key = ftok("shmfile", 65);
    int shm_id = shmget(shm_key, SHM_SIZE, 0666);
    if (shm_id == -1) {
        perror("shmget");
        exit(1);
    }

    char *shared_mem = (char*) shmat(shm_id, NULL, 0);
    if (shared_mem == (char *)(-1)) {
        perror("shmat");
        exit(1);
    }

    pid_t receiver_pid = getpid();
    time_t current_time;
    struct tm *time_info;
    char time_str[80];

    while(1) {
        time(&current_time);
        time_info = localtime(&current_time);
        strftime(time_str, sizeof(time_str), "%Y-%m-%d %H:%M:%S", time_info);
        printf("ReceiverTime: %s | ReceiverPID: %d | Received: %s", time_str, receiver_pid, shared_mem);
        sleep(1);
    }

    shmdt(shared_mem);
    return 0;
}