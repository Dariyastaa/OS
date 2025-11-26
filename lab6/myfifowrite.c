#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <time.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <string.h>
#include <errno.h>

const size_t size = 64;

char* getStrTime() {
    time_t ts = time(NULL);
    struct tm curTime = *localtime(&ts);
    char* timeStr = (char*)calloc(1, size);
    strftime(timeStr, size, "%H:%M:%S;", &curTime);
    return timeStr;
}

int main(void) {
    const char* fifoName = "fifoLab";

    if (mkfifo(fifoName, S_IRUSR | S_IWUSR) != 0 && errno != EEXIST) {
        fprintf(stderr, "mkfifo error: %s (%d)\n", strerror(errno), errno);
        return 1;
    }

    int fd = open(fifoName, O_WRONLY);
    if (fd == -1) {
        perror("open for write");
        return 1;
    }

    char* timeStr = getStrTime();
    char buf[2 * 64];

    snprintf(buf, sizeof(buf), "Sender PID: %d; Time: %s\n", getpid(), timeStr);
    free(timeStr);

    // сразу отправили строку с ТЕКУЩИМ временем
    write(fd, buf, strlen(buf) + 1);
    close(fd);

    return 0;
}