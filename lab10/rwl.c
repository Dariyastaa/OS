#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>

#define READERS 10
#define BUF_SIZE 128

static char shared_buf[BUF_SIZE];
static pthread_rwlock_t rwlock;
static volatile sig_atomic_t running = 1;

static void on_sigint(int sig) {
    (void)sig;
    running = 0;
}

static void* writer_thread(void* arg) {
    (void)arg;
    unsigned long counter = 0;

    while (running) {
        counter++;

        pthread_rwlock_wrlock(&rwlock);
        snprintf(shared_buf, BUF_SIZE, "record=%lu", counter);
        pthread_rwlock_unlock(&rwlock);

        sleep(1);
    }
    return NULL;
}

static void* reader_thread(void* arg) {
    long idx = (long)arg;

    unsigned long tid = (unsigned long)pthread_self();

    while (running) {
        char local_copy[BUF_SIZE];

        pthread_rwlock_rdlock(&rwlock);
        strncpy(local_copy, shared_buf, BUF_SIZE);
        local_copy[BUF_SIZE - 1] = '\0';
        printf("reader[%ld] tid=%lu => \"%s\"\n", idx, tid, local_copy);
        fflush(stdout);
        pthread_rwlock_unlock(&rwlock);

        usleep(800 * 1000);
    }
    return NULL;
}

int main(void) {
    signal(SIGINT, on_sigint);

    snprintf(shared_buf, BUF_SIZE, "record=0");

    if (pthread_rwlock_init(&rwlock, NULL) != 0) {
        perror("pthread_rwlock_init");
        return 1;
    }

    pthread_t writer;
    pthread_t readers[READERS];

    if (pthread_create(&writer, NULL, writer_thread, NULL) != 0) {
        perror("pthread_create writer");
        pthread_rwlock_destroy(&rwlock);
        return 1;
    }

    for (long i = 0; i < READERS; i++) {
        if (pthread_create(&readers[i], NULL, reader_thread, (void*)i) != 0) {
            perror("pthread_create reader");
            running = 0;
            break;
        }
    }

    pthread_join(writer, NULL);
    for (int i = 0; i < READERS; i++) pthread_join(readers[i], NULL);

    pthread_rwlock_destroy(&rwlock);
    return 0;
}