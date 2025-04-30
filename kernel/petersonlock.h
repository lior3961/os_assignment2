#include "types.h"

// Long-term locks for processes
struct petersonlock {
  
  uint active;       // Is the lock held?
  uint flag[2];
  uint turn;
      
  // For debugging:
  int lock_index;     // index of lock in the array.
  int pid;           // Process holding lock
};

void peterson_init(void);
int peterson_create(void);
int peterson_acquire(int lock_id, int role);
int peterson_release(int lock_id, int role);
int peterson_destroy(int lock_id);