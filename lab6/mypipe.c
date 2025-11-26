#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <string.h>
#include <time.h>
#include <stdlib.h>
#include <sys/wait.h>

#define BUFFER_SIZE 256

int main() {
    int fd[2];
    pid_t pid;
    char buffer[BUFFER_SIZE];
    if (pipe(fd) == -1) {
        perror("pipe");
        exit(EXIT_FAILURE);
    }
    pid = fork();

    if (pid > 0) {
        close(fd[0]);
        time_t parent_time = time(NULL);
        pid_t parent_pid = getpid();
        char message[BUFFER_SIZE];
        snprintf(message, BUFFER_SIZE, "Parent PID: %d, Time: %s", parent_pid, ctime(&parent_time));
        write(fd[1], message, strlen(message) + 1);
        close(fd[1]);
        wait(NULL);
    } else if (pid == 0) {
        close(fd[1]);
        read(fd[0], buffer, BUFFER_SIZE);
        sleep(5);
        time_t child_time = time(NULL);
        pid_t child_pid = getpid();
        printf("Child PID: %d, Time: %s", child_pid, ctime(&child_time));
        printf("Received message: %s", buffer);
        close(fd[0]);
        exit(EXIT_SUCCESS);
    } else {
        perror("fork");
        exit(EXIT_FAILURE);
    }

    return 0;
}