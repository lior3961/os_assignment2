#include "user.h"
#include "kernel/types.h"
#include "stddef.h"
#include "kernel/param.h"

int main(int argc, char *argv[]) {
    if (argc != 2) {
        printf("Usage: tournament <num_processes>\n");
        exit(1);
    }

    int num_processes = atoi(argv[1]);

    int id = tournament_create(num_processes);

    if (id < 0) {
        printf("Error: Failed to create tournament\n");
        exit(1);
    }    

    if (tournament_acquire() < 0) {
        printf("Process %d (ID %d): Failed to acquire lock\n", getpid(), id);
        exit(1);
    }

    // CRITICAL SECTION
    printf("Process pid: %d (Tournament ID %d) entered critical section\n", getpid(), id);
    sleep(3); // Simulate some work in the critical section

    if (tournament_release() < 0) {
        printf("Process %d (ID %d): Failed to release lock\n", getpid(), id);
        exit(1);
    }

    // Only the parent process should wait for all children
    if (id == 0) {
        // Wait for all children to complete
        for (int i = 1; i < num_processes; i++) {
            wait(0);
        }
        printf("All processes have completed\n");
    }

    exit(0);

}