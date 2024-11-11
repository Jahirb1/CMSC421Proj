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
    printf("App %d: check_temp_buffer started\n", app_id + 1);

    // TODO: Open log files for writing
    // Continuously check my_temp_buffer for new messages
    // Process messages and log the output
    // Implement exit conditions and cleanup
}

void run_application(int id, struct temp_buffer *temp_buffers) {
    app_id = id - 1;
    my_temp_buffer = &temp_buffers[app_id];

    printf("Application %d started, temp buffer address: %p\n", app_id + 1, (void*)my_temp_buffer);

    // TODO: Create a thread that runs check_temp_buffer
    // Wait for the thread to finish
    // Perform any necessary cleanup
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
