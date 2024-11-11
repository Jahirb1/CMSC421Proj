#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <time.h>
#include "app.h"
#include "buffer.h"

static int app_id;
static struct temp_buffer *my_temp_buffer;
static int should_exit = 0;

void *check_temp_buffer(void *arg) {
    (void)arg; // Suppress unused parameter warning
    printf("App %d: Monitoring temporary buffer for messages.\n", app_id + 1);

    // Prepare log file names
    char log_filename[64];
    snprintf(log_filename, sizeof(log_filename), "./app%d_log.txt", app_id + 1);

    // Open the application-specific log file
    FILE *app_log = fopen(log_filename, "a");
    if (!app_log) {
        perror("Error opening app-specific log file");
        pthread_exit(NULL);
    }

    // Open the shared log file for all applications
    FILE *all_apps_log = fopen("./all_apps_log.txt", "a");
    if (!all_apps_log) {
        perror("Error opening all-apps log file");
        fclose(app_log);
        pthread_exit(NULL);
    }

    while (!should_exit) {
        // Lock the temporary buffer to safely access shared data
        pthread_mutex_lock(&my_temp_buffer->mutex);
        if (my_temp_buffer->count > 0) {
            // Retrieve the message from the buffer
            struct message msg = my_temp_buffer->messages[--my_temp_buffer->count];
            pthread_mutex_unlock(&my_temp_buffer->mutex);

            // Get the current time for logging purposes
            time_t now = time(NULL);
            char time_str[32];
            strftime(time_str, sizeof(time_str), "%Y-%m-%d %H:%M:%S", localtime(&now));

            // Write log entries to both app-specific and all-apps log files
            fprintf(app_log, "[%s] App %d processed message: Priority: %d, Interval: %d ms, Data: %s\n", 
                    time_str, app_id + 1, msg.priority, msg.random_time_interval, msg.data);
            fprintf(all_apps_log, "[%s] App %d processed message: Priority: %d, Interval: %d ms, Data: %s\n", 
                    time_str, app_id + 1, msg.priority, msg.random_time_interval, msg.data);

            // Flush logs to ensure they are written to disk immediately
            fflush(app_log);
            fflush(all_apps_log);
        } else {
            // No messages to process, release the lock and wait
            pthread_mutex_unlock(&my_temp_buffer->mutex);
            sleep(3); // Sleep for 3 seconds before checking again
        }
    }

    // Close log files before exiting the thread
    fclose(app_log);
    fclose(all_apps_log);
    printf("App %d finished monitoring temporary buffer.\n", app_id + 1);
    pthread_exit(NULL);
}

void run_application(int id, struct temp_buffer *temp_buffers) {
    app_id = id - 1;
    my_temp_buffer = &temp_buffers[app_id];

    // Print a message indicating the start of the application
    printf("Application %d started, temp buffer address: %p\n", app_id + 1, (void*)my_temp_buffer);

    // Create a thread to monitor the temporary buffer for messages
    pthread_t buffer_thread;
    if (pthread_create(&buffer_thread, NULL, check_temp_buffer, NULL) != 0) {
        perror("Failed to create buffer monitoring thread");
        exit(EXIT_FAILURE);
    }

    // Wait for the buffer monitoring thread to finish
    pthread_join(buffer_thread, NULL);

    // Print a message indicating the application is exiting
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
