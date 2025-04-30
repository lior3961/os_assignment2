#include "types.h"
#include "riscv.h"
#include "defs.h"
#include "param.h"
#include "memlayout.h"
#include "spinlock.h"
#include "proc.h"
#include "sleeplock.h"
#include "petersonlock.h"


#define MAX_PETERSON_LOCKS 15
// Array of locks
struct petersonlock locks[MAX_PETERSON_LOCKS];

void
peterson_init(void)
{
  for (int i = 0; i < MAX_PETERSON_LOCKS; i++) {
    locks[i].active = 0;
    locks[i].flag[0] = 0;
    locks[i].flag[1] = 0;
    locks[i].turn = 0;
    locks[i].pid = -1;
  }
}

// Create a new peterson lock
int
peterson_create(void)
{
  for (int i = 0; i < MAX_PETERSON_LOCKS; i++) {
    if (__sync_lock_test_and_set(&locks[i].active, 1) == 0) {
      locks[i].lock_index = i;
      return i;
    }
  }
  return -1;
}



int
peterson_acquire(int lock_id, int role)
{
    if (lock_id < 0 || lock_id >= MAX_PETERSON_LOCKS || (role != 0 && role != 1)) {
        return -1; // Invalid lock ID or role
    }

    push_off(); // disable interrupts to avoid deadlock.
    __sync_synchronize();

    //lock wasn't active - it was never created
    // or it was destroyed
    if (locks[lock_id].active == 0) {
        pop_off();
        return -1; 
    }

    locks[lock_id].flag[role] = 1;
    locks[lock_id].turn = role;
    locks[lock_id].pid = myproc()->pid;

    __sync_synchronize();
    
    
    // Wait until the other process is not interested in the lock and it's our turn
    while(locks[lock_id].flag[1-role] && locks[lock_id].turn == role) {
        yield();
    }
    pop_off();

    return 0;
}


int 
peterson_release(int lock_id, int role)
{
    if (lock_id < 0 || lock_id >= MAX_PETERSON_LOCKS || (role != 0 && role != 1)) {
        return -1; // Invalid lock ID or role
    }

    push_off(); // disable interrupts to avoid deadlock.
    __sync_synchronize();

    //lock wasn't active - it was never created
    // or it was destroyed
    if (locks[lock_id].active == 0) {
        pop_off();
        return -1; 
    }

    locks[lock_id].flag[role] = 0;
    locks[lock_id].pid = -1;

    __sync_synchronize();
    pop_off();
    return 0;
}

int 
peterson_destroy(int lock_id) 
{
    if (lock_id < 0 || lock_id >= MAX_PETERSON_LOCKS) {
        return -1; // Invalid lock ID
    }

    push_off();
    __sync_synchronize();

    // Check if the lock is active and mark it as inactive atomically
    // This prevents new acquisitions immediately
    if (__sync_lock_test_and_set(&locks[lock_id].active, 0) == 0) {
        pop_off();
        return -1; // Lock was already inactive
    }

    // Now that the lock is inactive, clean up the other fields
    // Any process checking .active will already see it's invalid
    locks[lock_id].flag[0] = 0;
    locks[lock_id].flag[1] = 0;
    locks[lock_id].turn = 0;

    __sync_synchronize();
    pop_off();

    return 0;
}