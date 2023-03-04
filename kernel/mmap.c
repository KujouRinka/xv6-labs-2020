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

uint64
sys_munmap(void) {
  // int munmap(void *addr, size_t length);
  // TODO:
  void *addr;
  uint length;
  if (argaddr(0, (uint64 *) &addr) < 0)
    return -1;
  if (argaddr(1, (uint64 *) &length) < 0)
    return -1;
  return -1;
}
