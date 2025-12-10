#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>

#define READERS 10

pthread_mutex_t mtx = PTHREAD_MUTEX_INITIALIZER;
char shared_buf[64] = "start";
int counter = 0;
int stop = 0;

void* writer(void* arg) {
    for (int i = 0; i < 20; i++) {
        pthread_mutex_lock(&mtx);
        counter++;
        snprintf(shared_buf, sizeof(shared_buf), "Запись № %d", counter);
        pthread_mutex_unlock(&mtx);
        usleep(200000);
    }
    pthread_mutex_lock(&mtx);
    stop = 1;
    pthread_mutex_unlock(&mtx);
    return NULL;
}

void* reader(void* arg) {
    long id = (long)arg;
    while (1) {
        pthread_mutex_lock(&mtx);
        if (stop) {
            pthread_mutex_unlock(&mtx);
            break;
        }
        printf("reader %ld: %s\n", id, shared_buf);
        pthread_mutex_unlock(&mtx);
        usleep(100000);
    }
    return NULL;
}

int main(void) {
    pthread_t w;
    pthread_t r[READERS];

    pthread_create(&w, NULL, writer, NULL);
    for (long i = 0; i < READERS; i++)
        pthread_create(&r[i], NULL, reader, (void*)i);

    pthread_join(w, NULL);
    for (int i = 0; i < READERS; i++)
        pthread_join(r[i], NULL);

    pthread_mutex_destroy(&mtx);
    return 0;
}