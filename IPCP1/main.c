#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <pthread.h>
#include "buffer.h"
#include "app.h"

struct temp_buffer *shared_temp_buffers;

void create_application_process(int app_id) {
    // TODO: Implement process creation using fork()
    // The child process should call run_application(app_id, shared_temp_buffers)
}

int main() {
    printf("Main program started\n");
    
    // Create shared memory for temp buffers
    int shm_fd = shm_open("/temp_buffers", O_CREAT | O_RDWR, 0666);
    if (shm_fd == -1) {
        perror("shm_open");
        exit(1);
    }

    if (ftruncate(shm_fd, NUM_APPS * sizeof(struct temp_buffer)) == -1) {
        perror("ftruncate");
        exit(1);
    }

    shared_temp_buffers = mmap(0, NUM_APPS * sizeof(struct temp_buffer), 
                               PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
    if (shared_temp_buffers == MAP_FAILED) {
        perror("mmap");
        exit(1);
    }

    buffer_init(shared_temp_buffers);
    printf("Buffer initialized\n");

    // Create application processes
    for (int i = 1; i <= NUM_APPS; i++) {
        create_application_process(i);
    }

    pthread_t producer, consumer;
    printf("Creating producer thread\n");
    if (pthread_create(&producer, NULL, producer_function, shared_temp_buffers) != 0) {
        perror("Failed to create producer thread");
        exit(1);
    }
    
    printf("Creating consumer thread\n");
    if (pthread_create(&consumer, NULL, consumer_function, shared_temp_buffers) != 0) {
        perror("Failed to create consumer thread");
        exit(1);
    }

    // Wait for all child processes to finish
    for (int i = 0; i < NUM_APPS; i++) {
        int status;
        wait(&status);
        printf("Child process %d exited with status %d\n", i+1, WEXITSTATUS(status));
    }

    printf("All child processes finished, signaling threads to stop\n");
    should_stop = true;

    // Post semaphores to unblock threads
    sem_post(&main_buffer.full);
    sem_post(&main_buffer.empty);

    printf("Waiting for producer thread to finish\n");
    pthread_join(producer, NULL);
    
    printf("Waiting for consumer thread to finish\n");
    pthread_join(consumer, NULL);

    printf("Cleaning up buffer\n");
    buffer_cleanup(shared_temp_buffers);

    // Clean up shared memory
    munmap(shared_temp_buffers, NUM_APPS * sizeof(struct temp_buffer));
    close(shm_fd);
    shm_unlink("/temp_buffers");

    printf("Main program finished\n");
    return 0;
}
