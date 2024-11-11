/* app.c */
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
    (void)arg; // Avoid unused parameter warning

    // Check for messages in the temporary buffer every 3 seconds
    printf("App %d: Monitoring temporary buffer for messages.\n", app_id);

    char log_filename[64];
    snprintf(log_filename, sizeof(log_filename), "/var/log/IPCproject/app%d_log.txt", app_id);

    FILE *app_log = fopen(log_filename, "a");
    if (!app_log) {
        perror("Error opening app log file");
        pthread_exit(NULL);
    }

    FILE *all_apps_log = fopen("/var/log/IPCproject/all_apps_log.txt", "a");
    if (!all_apps_log) {
        perror("Error opening all-apps log file");
        fclose(app_log);
        pthread_exit(NULL);
    }

    while (!should_exit) {
        pthread_mutex_lock(&my_temp_buffer->mutex);
        if (my_temp_buffer->count > 0) {
            struct message msg = my_temp_buffer->messages[--my_temp_buffer->count];
            pthread_mutex_unlock(&my_temp_buffer->mutex);

            time_t now = time(NULL);
            char time_str[32];
            strftime(time_str, sizeof(time_str), "%Y-%m-%d %H:%M:%S", localtime(&now));

            // Log the message processing
            fprintf(app_log, "[%s] Processed message: Priority: %d, Interval: %d ms, Data: %s\n", time_str, msg.priority, msg.random_time_interval, msg.data);
            fprintf(all_apps_log, "[%s] App %d: Processed message: Priority: %d, Interval: %d ms, Data: %s\n", time_str, app_id, msg.priority, msg.random_time_interval, msg.data);

            fflush(app_log);
            fflush(all_apps_log);
        } else {
            pthread_mutex_unlock(&my_temp_buffer->mutex);
            sleep(3);
        }
    }

    fclose(app_log);
    fclose(all_apps_log);
    pthread_exit(NULL);
}

void run_application(int id, struct temp_buffer *temp_buffers) {
    app_id = id;
    my_temp_buffer = &temp_buffers[app_id - 1];

    pthread_t buffer_thread;
    if (pthread_create(&buffer_thread, NULL, check_temp_buffer, NULL) != 0) {
        perror("Error creating thread for monitoring temp buffer");
        exit(1);
    }

    pthread_join(buffer_thread, NULL);
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
