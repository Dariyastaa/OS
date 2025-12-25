#include <pthread.h>
#include <semaphore.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>

#define BUF_SIZE 128

static char shared_buf[BUF_SIZE];
static sem_t sem;
static volatile sig_atomic_t running = 1;

static void on_sigint(int signo) {
    (void)signo;
    running = 0;
}

static void* writer_thread(void* arg) {
    (void)arg;
    unsigned long counter = 0;

    while (running) {
        counter++;

        if (sem_wait(&sem) != 0) {
            perror("sem_wait (writer)");
            break;
        }

        snprintf(shared_buf, BUF_SIZE, "record=%lu", counter);

        if (sem_post(&sem) != 0) {
            perror("sem_post (writer)");
            break;
        }

        sleep(1);
    }

    return NULL;
}

static void* reader_thread(void* arg) {
    (void)arg;

    unsigned long tid = (unsigned long)pthread_self();

    while (running) {
        char local_copy[BUF_SIZE];

        if (sem_wait(&sem) != 0) {
            perror("sem_wait (reader)");
            break;
        }

        strncpy(local_copy, shared_buf, BUF_SIZE);
        local_copy[BUF_SIZE - 1] = '\0';

        if (sem_post(&sem) != 0) {
            perror("sem_post (reader)");
            break;
        }

        printf("reader tid=%lu, shared_buf=\"%s\"\n", tid, local_copy);
        fflush(stdout);

        usleep(200 * 1000);
    }

    return NULL;
}

int main(void) {
    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = on_sigint;
    sigaction(SIGINT, &sa, NULL);

    snprintf(shared_buf, BUF_SIZE, "record=0");

    if (sem_init(&sem, 0, 1) != 0) {
        perror("sem_init");
        return 1;
    }

    pthread_t w, r;

    if (pthread_create(&w, NULL, writer_thread, NULL) != 0) {
        perror("pthread_create (writer)");
        sem_destroy(&sem);
        return 1;
    }

    if (pthread_create(&r, NULL, reader_thread, NULL) != 0) {
        perror("pthread_create (reader)");
        running = 0;
        pthread_join(w, NULL);
        sem_destroy(&sem);
        return 1;
    }

    pthread_join(w, NULL);
    pthread_join(r, NULL);

    sem_destroy(&sem);
    puts("Done.");
    return 0;
}