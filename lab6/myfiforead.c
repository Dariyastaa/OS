#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <time.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <string.h>
#include <errno.h>

const size_t size = 128;

int main(void) {
    const char* fifoName = "fifoLab";

    int fd = open(fifoName, O_RDONLY);
    if (fd == -1) {
        perror("open for read");
        return 1;
    }

    char* buf = (char*)calloc(1, size);
    if (!buf) {
        perror("calloc");
        close(fd);
        return 1;
    }

    ssize_t n = read(fd, buf, size - 1);
    if (n <= 0) {
        perror("read");
        free(buf);
        close(fd);
        return 1;
    }
    buf[n] = '\0';
    close(fd);

    // ждём 10+ секунд перед тем, как зафиксировать СВОЁ время
    sleep(10);

    time_t ts = time(NULL);
    struct tm curTime = *localtime(&ts);
    char timeStr[64];
    strftime(timeStr, sizeof(timeStr), "%H:%M:%S;", &curTime);

    printf("Receiver PID: %d; Time: %s\n", getpid(), timeStr);
    printf("Received: %s", buf);

    free(buf);
    return 0;
}