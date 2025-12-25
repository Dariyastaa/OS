#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>

#define READERS 10
#define BUF_SIZE 128

static char shared_buf[BUF_SIZE];

static pthread_mutex_t mtx = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t  cv  = PTHREAD_COND_INITIALIZER;

static unsigned long version = 0;
static volatile sig_atomic_t running = 1;

static void on_sigint(int sig) {
    (void)sig;
    running = 0;
    pthread_mutex_lock(&mtx);
    pthread_cond_broadcast(&cv);
    pthread_mutex_unlock(&mtx);
}

static void* writer_thread(void* arg) {
    (void)arg;
    unsigned long counter = 0;

    while (running) {
        sleep(1);
        counter++;

        pthread_mutex_lock(&mtx);
        snprintf(shared_buf, BUF_SIZE, "record=%lu", counter);
        version++;
        pthread_cond_broadcast(&cv);
        pthread_mutex_unlock(&mtx);
    }
    return NULL;
}

static void* reader_thread(void* arg) {
    long idx = (long)arg;
    unsigned long tid = (unsigned long)pthread_self();
    unsigned long my_version = 0;

    while (1) {
        pthread_mutex_lock(&mtx);

        while (running && my_version == version) {
            pthread_cond_wait(&cv, &mtx);
        }

        if (!running) {
            pthread_mutex_unlock(&mtx);
            break;
        }

        my_version = version;
        char local_copy[BUF_SIZE];
        strncpy(local_copy, shared_buf, BUF_SIZE);
        local_copy[BUF_SIZE - 1] = '\0';

        pthread_mutex_unlock(&mtx);

        printf("reader[%ld] tid=%lu => \"%s\"\n", idx, tid, local_copy);
        fflush(stdout);
    }

    return NULL;
}

int main(void) {
    signal(SIGINT, on_sigint);

    pthread_t writer;
    pthread_t readers[READERS];

    pthread_mutex_lock(&mtx);
    snprintf(shared_buf, BUF_SIZE, "record=0");
    version = 1;
    pthread_mutex_unlock(&mtx);

    if (pthread_create(&writer, NULL, writer_thread, NULL) != 0) {
        perror("pthread_create writer");
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

    pthread_mutex_destroy(&mtx);
    pthread_cond_destroy(&cv);
    return 0;
}