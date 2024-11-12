#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <time.h>
#include "app.h"
#include "buffer.h"
#include <string.h>

static int app_id;
static struct temp_buffer *my_temp_buffer;
static int should_exit = 0;

void check_temp_buffer(void *arg) {
    (void)arg; // Marking param as unused to avoid warnings
    // Each app will process 16 messages (48 total, divided among 3 apps)
    const int MAX_MESSAGES = 16; 
    printf("App %d: check_temp_buffer started\n", app_id + 1);

    FILE *app_log,*all_apps_log;
    char app_log_filename[64];
    snprintf(app_log_filename, sizeof(app_log_filename), "/var/log/IPCproject/app%d_log.txt", app_id + 1);
    // Oppen both log files and check if theres anything in them
    app_log = fopen(app_log_filename, "a");
    if (!app_log) {
        perror("Error opening app-specific log file");
        pthread_exit(NULL);
    }

    all_apps_log = fopen("/var/log/IPCproject/all_apps_log.txt", "a");
    if (!all_apps_log) {
        perror("Error opening all-apps log file");
        fclose(app_log);
        pthread_exit(NULL);
    }
    int processed_messages = 0; // Counter to keep track of the messages processed

    while (!should_exit) {
        pthread_mutex_lock(&my_temp_buffer->mutex);
        if (my_temp_buffer->count > 0) {
            struct buffer_item msg = my_temp_buffer->items[--my_temp_buffer->count];
            pthread_mutex_unlock(&my_temp_buffer->mutex);

            time_t now = time(NULL);
            char *timestamp = ctime(&now);
            timestamp[strlen(timestamp) - 1] = '\0'; 
            // Formatted messages for the consolse including the timestamp,priority, and msgpriority
            printf("[%s] App %d processed message: Priority %d, Interval %d ms, Data: %s\n",timestamp, app_id + 1, msg.priority, msg.random_time_interval, msg.data);
            fprintf(app_log, "[%s] App%d - Priority: %d, Interval: %d ms, Data: %s\n",timestamp, app_id + 1, msg.priority, msg.random_time_interval, msg.data);
            fprintf(all_apps_log, "[%s] App%d - Priority: %d, Interval: %d ms, Data: %s\n",timestamp, app_id + 1, msg.priority, msg.random_time_interval, msg.data);
            
            // Ensure we've written to both log files
            fflush(app_log);
            fflush(all_apps_log);
            // Increment the number of processed messages
            processed_messages++;

            // Check if all messages have been processed for this app
            if (processed_messages >= MAX_MESSAGES) {
                printf("App %d has processed all %d messages.\n", app_id + 1, MAX_MESSAGES);
                should_exit = 1; // Set flag to true (ie 1)
            }

        } 
        else {
            pthread_mutex_unlock(&my_temp_buffer->mutex);
            // Sleep if there aren't any messages
            sleep(3);
        }
    }
    // Close the files and print that info to the console
    fclose(app_log);
    fclose(all_apps_log);
    printf("App %d finished processing.\n", app_id + 1);
    // Then exit
    pthread_exit(NULL);
}

void run_application(int id, struct temp_buffer *temp_buffers) {
    app_id = id - 1;
    my_temp_buffer = &temp_buffers[app_id];

    printf("Application %d started, temp buffer address: %p\n", app_id + 1, (void*)my_temp_buffer);
    // TODO: Create a thread that runs check_temp_buffer
    pthread_t buffer_thread;
    if (pthread_create(&buffer_thread, NULL, check_temp_buffer, NULL) != 0) {
        perror("Failed to create buffer checking thread");
        return;
    }
    // Wait for the thread to finish
    pthread_join(buffer_thread, NULL);
    // Perform any necessary cleanup
    printf("Application %d exiting\n", app_id + 1);
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <app_id>\n", argv[0]);
        exit(1);
    }
    int app_id = atoi(argv[1]);  // Convert the app_id argument from string to integer
    printf("App started with app_id: %d\n", app_id);

    // Attach to the shared memory segment created in main.c
    int shm_fd = shm_open("/temp_buffers", O_RDWR, 0666);  // Open the shared memory region
    if (shm_fd == -1) {
        perror("shm_open");
        exit(1);
    }

    shared_temp_buffers = mmap(0, NUM_APPS * sizeof(struct temp_buffer), 
                               PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
    if (shared_temp_buffers == MAP_FAILED) {
        perror("mmap");
        exit(1);
    }

    // Run the application logic using the app_id
    run_application(app_id, shared_temp_buffers);

    return 0;
}
