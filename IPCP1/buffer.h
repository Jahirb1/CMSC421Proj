#ifndef BUFFER_H
#define BUFFER_H

#include <semaphore.h>
#include <pthread.h>
#include <stdbool.h>

#define BUFFER_SIZE 10
#define TEMP_BUFFER_SIZE 16
#define MAX_DATA_LEN 256
#define NUM_APPS 3

struct buffer_item {
    int app_id;
    int priority;
    int random_time_interval;
    char data[MAX_DATA_LEN];
    time_t timestamp;
};

struct circular_buffer {
    struct buffer_item items[BUFFER_SIZE];
    int read, write, count;
    sem_t mutex, full, empty;
};

struct temp_buffer {
    struct buffer_item items[TEMP_BUFFER_SIZE];
    int count;
    pthread_mutex_t mutex;
};

extern struct circular_buffer main_buffer;
extern bool should_stop;

void buffer_init(struct temp_buffer *temp_buffers);
void buffer_cleanup(struct temp_buffer *temp_buffers);
void *producer_function(void *arg);
void *consumer_function(void *arg);

#endif
