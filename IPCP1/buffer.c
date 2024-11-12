#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "buffer.h"

struct circular_buffer main_buffer;
bool should_stop = false;

void buffer_init(struct temp_buffer *temp_buffers) {
    // TODO: Initialize the main_buffer and temp_buffers
    main_buffer.read = 0;
    main_buffer.write = 0;
    main_buffer.count = 0;
    // Initialize semaphores and mutexes for the buffer 
    sem_init(&main_buffer.empty, 0, BUFFER_SIZE);  
    sem_init(&main_buffer.full, 0, 0); 
    sem_init(&main_buffer.mutex, 0, 1);
    // Initialize temp_buffers' mutexes
    for (int i = 0; i < NUM_APPS; i++) {
        pthread_mutex_init(&temp_buffers[i].mutex, NULL);
        temp_buffers[i].count = 0;
    }  
}

void buffer_cleanup(struct temp_buffer *temp_buffers) {
    // TODO: Destroy semaphores and mutexes
    sem_destroy(&main_buffer.empty); // Destroying the mutexes and semaphores at their address 
    sem_destroy(&main_buffer.full);
    sem_destroy(&main_buffer.mutex);
    // Loop through the list and destroy each temp_buffers mutex
    for (int i = 0; i < NUM_APPS; i++)
        pthread_mutex_destroy(&temp_buffers[i].mutex);
}

void bubbleSort(int arr[], int n) {
    // Simple bubble sort nested loop to simulate randomisation 
    int temp;
    for (int i = 0; i < n-1; i++) {
        for (int j = 0; j < n-i-1; j++) {
            if (arr[j] > arr[j+1]) {
                temp = arr[j];
                arr[j] = arr[j+1];
                arr[j+1] = temp;
            }
        }
    }
}


void *producer_function(void *arg) {
    // struct temp_buffer *temp_buffers = (struct temp_buffer *)arg;
    printf("Producer function started\n");
    FILE *input_file = fopen("input.txt", "r");
    if (!input_file) {
        perror("Failed to open input file");
        return NULL;
    }
    // Intialise array to store each line of the file
    char line[MAX_DATA_LEN];
    bool continue_producing = true; // Flag to control the while loop
        while (continue_producing) {
        if (fgets(line, sizeof(line), input_file)) {
            struct buffer_item item;
            sscanf(line, "app%d %d %d %[^\n]", &item.app_id, &item.priority, &item.random_time_interval, item.data);
            item.timestamp = time(NULL);

            // Wait for an empty slot in the circular buffer
            sem_wait(&main_buffer.empty);  
            // Lock the buffer to add an item
            sem_wait(&main_buffer.mutex);  

            // Add the item to the circular buffer
            main_buffer.items[main_buffer.write] = item;
            main_buffer.write = (main_buffer.write + 1) % BUFFER_SIZE;
            main_buffer.count++;

            // Print timestamped log for each produced item
            time_t now = time(NULL);
            char *timestamp = ctime(&now);
            timestamp[strlen(timestamp) - 1] = '\0';  // Remove newline character
            printf("[%s] Producer: Added message for app %d with Priority %d, Interval %d ms, Data: %s\n",timestamp, item.app_id, item.priority, item.random_time_interval, item.data);
            // Unlock the buffer
            sem_post(&main_buffer.mutex);  
            // Then signal that a full slot is available
            sem_post(&main_buffer.full);   

        } 
        else
            // End of the file reached, stop producing
            continue_producing = false;
    }

    fclose(input_file);
    should_stop = true;
    sem_post(&main_buffer.full);
    return NULL;
}



void *consumer_function(void *arg) {
    struct temp_buffer *temp_buffers = (struct temp_buffer *)arg;
    printf("Consumer function started\n");

    while (1) { // while we have permission to go  
        // Wait for a full slot
        sem_wait(&main_buffer.full);  
        // Lock the buffer to avoid race conditions
        sem_wait(&main_buffer.mutex); 
        // Check if `should_stop` and the buffer is empty to exit
        if (should_stop && main_buffer.count == 0) {
            sem_post(&main_buffer.mutex); // Release the mutex before exiting
            sem_post(&main_buffer.full);  // Signal any other consumers waiting to exit
            return NULL; // Exit the function
        } 
        if (main_buffer.count > 0) {
            // Remove an item from the circular buffer (ie let the consumer feed)
            struct buffer_item item = main_buffer.items[main_buffer.read];
            main_buffer.read = (main_buffer.read + 1) % BUFFER_SIZE;
            main_buffer.count--;
            // Once the consumer is done feeding we can unlock the buffer
            sem_post(&main_buffer.mutex);
            // And now that we know that the buffer has an empty slot we now want to signal that
            sem_post(&main_buffer.empty);
            // Print timestamped log for each produced item
            time_t now = time(NULL);
            char *timestamp = ctime(&now);
            timestamp[strlen(timestamp) - 1] = '\0';  // Remove newline character
            printf("[%s] Consumer: Moved message to temp buffer for App %d [App: %d, Priority: %d, Data: %s]\n",timestamp, item.app_id, item.app_id, item.priority, item.data); 

            // Now we can determine which temp_buffer to place the item in based on its app_id
            pthread_mutex_lock(&temp_buffers[item.app_id - 1].mutex); //-1 since the array is 0 based
            temp_buffers[item.app_id - 1].items[temp_buffers[item.app_id - 1].count] = item;
            // Incriment how many items are stored so that future consumer threads can continue to pla items in the correct slot
            temp_buffers[item.app_id - 1].count++; 
            pthread_mutex_unlock(&temp_buffers[item.app_id - 1].mutex); // Unlock the mutex after the task is in the buffer so other threads can go through
        }
        else
            // Unlock if nothing was done, to avoid deadlocks
            sem_post(&main_buffer.mutex); 
    }

    return NULL;
}
