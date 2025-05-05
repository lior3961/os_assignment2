#include "user.h"
#include "kernel/types.h"
#include "stddef.h"
#include "kernel/param.h"

// Global variables to store tournament state
static int *locks = 0;       // Array to store lock IDs
static int num_processes = 0;// Number of processes in tournament
static int num_levels = 0;   // Number of levels in the tree
static int process_index = -1;// Index of this process in the tournament

int tournament_create(int processes) {
    // Validate that processes is a power of 2 and <= 16
    if (processes <= 0 || processes > 16 || (processes & (processes - 1)) != 0) {
        return -1;  // Invalid number of processes
    }

    int levels = 0;
    int temp = processes;
    while (temp > 1) {
        temp >>= 1;
        levels++;
    }

    int total_locks = processes - 1;
    
    // Allocate memory for locks
    locks = malloc(total_locks * sizeof(int));
    if(!locks) {
        return -1;  // Memory allocation failed
    }

    // Create all the locks needed for the tree
    for (int i = 0; i < total_locks; i++) {
        locks[i] = peterson_create();
        if (locks[i] < 0) {
            // Failed to create lock, cleanup and return error
            for (int j = 0; j < i; j++) {
                peterson_destroy(locks[j]);
            }
            free(locks);
            locks = 0;
            return -1;
        }
    }
    
    // Store tournament state
    num_processes = processes;
    num_levels = levels;

    process_index = 0;  // Initialize process index for the parent
    
    // Fork processes and assign each one an index
    for (int i = 1; i < processes; i++) {
        int pid = fork();
        if (pid < 0) {
            // Fork failed, return error
            return -1;
        } else if (pid == 0) {
            // Child process
            process_index = i;
            break;  // Exit the loop in the child process
        }
    }
    
    return process_index;  // Return the index assigned to this process
}

int tournament_acquire(void) {
    if (!locks || process_index < 0 || num_levels <= 0) {
        return -1;  // Tournament not created or invalid state
    }
    
    for(int l = num_levels - 1; l>=0; l--){
        int role_l = (process_index & (1 << (num_levels - l - 1))) >> (num_levels - l - 1);
        int lock_l = process_index >> (num_levels - l);
        int lock_index = lock_l+ (1 << l) - 1; 
        if (peterson_acquire(locks[lock_index], role_l) < 0) {
            // Failed to acquire lock, release previously acquired locks
            for (int r_level = l + 1; r_level < num_levels; r_level++) {
                int r_role = (process_index & (1 << (num_levels - r_level - 1))) >> (num_levels - r_level - 1);
                int r_lock_l = process_index >> (num_levels - r_level);
                int r_lock_index = r_lock_l + (1 << r_level) - 1;
                peterson_release(locks[r_lock_index], r_role);
            }
            return -1;
        }
    }

    return 0;
}

int tournament_release(void) {
    if (!locks || process_index < 0 || num_levels <= 0) {
        return -1;  // Tournament not created or invalid state
    }

    for(int l = 0; l < num_levels; l++){
        int role_l = (process_index & (1 << (num_levels - l - 1))) >> (num_levels - l - 1);
        int lock_l = process_index >> (num_levels - l);
        int lock_index = lock_l + (1 << l) - 1;
        if(peterson_release(locks[lock_index], role_l) < 0){
            return -1; // Failed to release lock
        }
    }
    
    return 0;
}
