#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "buffer.h"

#define BUFFER_SIZE 10

// Use the original struct definition
struct circular_buffer {
    struct buffer_item items[BUFFER_SIZE];
    int read;
    int write;
    int count;
    sem_t mutex;
    sem_t full;
    sem_t empty;
};

struct circular_buffer main_buffer;
bool should_stop = false;

void buffer_init(struct temp_buffer *temp_buffers) {
    // Initialize the main buffer
    main_buffer.read = 0;
    main_buffer.write = 0;
    main_buffer.count = 0;
    sem_init(&main_buffer.mutex, 0, 1);
    sem_init(&main_buffer.empty, 0, BUFFER_SIZE);
    sem_init(&main_buffer.full, 0, 0);

    // Initialize temporary buffers
    for (int i = 0; i < NUM_APPS; i++) {
        temp_buffers[i].count = 0;
        pthread_mutex_init(&temp_buffers[i].mutex, NULL);
    }
}

void buffer_cleanup(struct temp_buffer *temp_buffers) {
    // Clean up synchronization tools for the main buffer
    sem_destroy(&main_buffer.mutex);
    sem_destroy(&main_buffer.empty);
    sem_destroy(&main_buffer.full);

    // Clean up each temporary buffer's mutex
    for (int i = 0; i < NUM_APPS; i++) {
        pthread_mutex_destroy(&temp_buffers[i].mutex);
    }
}

void bubbleSort(int arr[], int n) {
    // Simple bubble sort to simulate delay
    for (int i = 0; i < n - 1; i++) {
        for (int j = 0; j < n - i - 1; j++) {
            if (arr[j] > arr[j + 1]) {
                int temp = arr[j];
                arr[j] = arr[j + 1];
                arr[j + 1] = temp;
            }
        }
    }
}

void *producer_function(void *arg) {
    struct temp_buffer *temp_buffers = (struct temp_buffer *)arg;
    printf("Producer thread started\n");

    FILE *input_file = fopen("input.txt", "r");
    if (!input_file) {
        perror("Unable to open input file");
        pthread_exit(NULL);
    }

    char line[256];
    while (fgets(line, sizeof(line), input_file) && !should_stop) {
        struct buffer_item item;
        sscanf(line, "%s %d %d %[^"]", item.app_id, &item.priority, &item.random_time_interval, item.data);

        // Simulate a processing delay using bubble sort
        int dummy_array[5] = {item.random_time_interval, 3, 1, 4, 2};
        bubbleSort(dummy_array, 5);

        // Wait until there is an empty slot in the buffer
        sem_wait(&main_buffer.empty);
        sem_wait(&main_buffer.mutex);

        // Add the item to the buffer
        main_buffer.items[main_buffer.write] = item;
        main_buffer.write = (main_buffer.write + 1) % BUFFER_SIZE;
        main_buffer.count++;

        // Release the lock and signal that there is a new item in the buffer
        sem_post(&main_buffer.mutex);
        sem_post(&main_buffer.full);

        // Log the addition of the item
        printf("Producer added item: App %s, Priority %d, Interval %d ms, Data: %s\n", 
               item.app_id, item.priority, item.random_time_interval, item.data);
    }

    fclose(input_file);
    pthread_exit(NULL);
}

void *consumer_function(void *arg) {
    struct temp_buffer *temp_buffers = (struct temp_buffer *)arg;
    printf("Consumer thread started\n");

    while (!should_stop) {
        // Wait until there is a full slot in the buffer
        sem_wait(&main_buffer.full);
        sem_wait(&main_buffer.mutex);

        // Remove an item from the buffer
        struct buffer_item item = main_buffer.items[main_buffer.read];
        main_buffer.read = (main_buffer.read + 1) % BUFFER_SIZE;
        main_buffer.count--;

        // Release the lock and signal that there is an empty slot available
        sem_post(&main_buffer.mutex);
        sem_post(&main_buffer.empty);

        // Determine which temporary buffer to use based on the app_id
        int target_buffer_index = atoi(&item.app_id[3]) - 1;  // Extract app number from app_id string
        if (target_buffer_index >= 0 && target_buffer_index < NUM_APPS) {
            pthread_mutex_lock(&temp_buffers[target_buffer_index].mutex);

            // Add the item to the appropriate temporary buffer
            temp_buffers[target_buffer_index].items[temp_buffers[target_buffer_index].count++] = item;

            // Log the movement of the item
            printf("Consumer moved item to App %s: Priority %d, Interval %d ms, Data: %s\n", 
                   item.app_id, item.priority, item.random_time_interval, item.data);

            pthread_mutex_unlock(&temp_buffers[target_buffer_index].mutex);
        }
    }

    pthread_exit(NULL);
}
