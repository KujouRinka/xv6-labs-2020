#ifndef __KERNEL_SPINLOCK_H__
#define __KERNEL_SPINLOCK_H__

// Mutual exclusion lock.
struct spinlock {
  uint locked;       // Is the lock held?

  // For debugging:
  char *name;        // Name of lock.
  struct cpu *cpu;   // The cpu holding the lock.
};

#endif
