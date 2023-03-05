#include "types.h"
#include "riscv.h"
#include "defs.h"
#include "param.h"
#include "spinlock.h"
#include "proc.h"
#include "fcntl.h"
#include "sleeplock.h"
#include "fs.h"
#include "file.h"

uint64
sys_mmap(void) {
  // void *mmap(void *addr, size_t length, int prot, int flags, int fd, off_t offset);
  void *addr;
  uint length;
  int prot;
  int flag;
  struct file *f;
  uint offset;
  if (argaddr(0, &addr) < 0)
    return -1;
  if (argint(1, &length) < 0)
    return -1;
  if (argint(2, &prot) < 0)
    return -1;
  if (argint(3, &flag) < 0)
    return -1;
  if (argfd(4, 0, &f) < 0)
    return -1;
  if (argint(5, &offset) < 0)
    return -1;
  if (!f->writable && (prot & PROT_WRITE) && (flag & MAP_SHARED))
    return -1;
  if (addr != 0)
    panic("mmap: addr != 0");
  // find free virtual space
  struct proc *me = myproc();
  for (int i = 0; i < VMA_SIZE; ++i) {
    if (me->vmas[i].used != 0)
      continue;
    struct vma *v = &me->vmas[i];
    v->addr = addr = (void *) me->sz;
    me->sz += length;
    v->length = length;
    v->prot = prot;
    v->flag = flag;
    v->offset = offset;
    filedup(f);
    v->f = f;
    v->used = 1;
    return (uint64) addr;
  }
  return 0xffffffffffffffff;
}

uint64 munmap_helper(void *addr, uint length) {
  struct proc *me = myproc();
  int i;
  for (i = 0; i < VMA_SIZE; ++i) {
    if (me->vmas[i].used == 0)
      continue;
    if (me->vmas[i].addr <= addr &&
        (uint64) addr < (uint64) me->vmas[i].addr + me->vmas[i].length)
      break;
  }
  if (i == VMA_SIZE)
    return -1;
  uint64 free_start = PGROUNDDOWN((uint64) addr);
  uint64 free_end = PGROUNDUP((uint64) addr + length);
  struct vma *v = &me->vmas[i];
  if (free_start == (uint64) v->addr) {
    v->addr = (void *) free_end;
    v->length -= free_end - free_start;
    v->offset += free_end - free_start;
  } else if (free_end == (uint64) v->addr + v->length) {
    v->length -= free_end - free_start;
  } else {
    panic("munmmap_helper: cannot munmap middle of a region");
  }
  // if MAP_SHARED write dirty
  if (v->flag & MAP_SHARED) {
    struct file *f = v->f;
    filewrite(f, free_start, free_end - free_start);
  }
  uvmunmap(me->pagetable, free_start, (free_end - free_start) / PGSIZE, 1);
  if (v->length == 0) {
    v->used = 0;
    fileclose(v->f);
  }

  return 0;
}

uint64
sys_munmap(void) {
  // int munmap(void *addr, size_t length);
  void *addr;
  uint length;
  if (argaddr(0, (uint64 *) &addr) < 0)
    return -1;
  if (argint(1, &length) < 0)
    return -1;
  return munmap_helper(addr, length);
}
