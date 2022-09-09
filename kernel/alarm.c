#include "types.h"
#include "riscv.h"
#include "defs.h"
#include "date.h"
#include "param.h"
#include "memlayout.h"
#include "spinlock.h"
#include "proc.h"

uint64 sys_sigalarm(void) {
  struct proc *me = myproc();
  me->alarm_interval = me->trapframe->a0;
  me->alarm_callback = me->trapframe->a1;
  me->alarm_tick_since_last_call = 0;
  return 0;
}

uint64 sys_sigreturn(void) {
  struct proc *me = myproc();
  if (me->trapframe_cp == 0)
    return -1;
  memmove(me->trapframe, me->trapframe_cp, sizeof(struct trapframe));
  me->alarm_tick_since_last_call = 0;
  me->alarm_callback_can_call = 1;
  return 0;
}
