// Buffer cache.
//
// The buffer cache is a linked list of buf structures holding
// cached copies of disk block contents.  Caching disk blocks
// in memory reduces the number of disk reads and also provides
// a synchronization point for disk blocks used by multiple processes.
//
// Interface:
// * To get a buffer for a particular disk block, call bread.
// * After changing buffer data, call bwrite to write it to disk.
// * When done with the buffer, call brelse.
// * Do not use the buffer after calling brelse.
// * Only one process at a time can use a buffer,
//     so do not keep them longer than necessary.


#include "types.h"
#include "param.h"
#include "spinlock.h"
#include "sleeplock.h"
#include "riscv.h"
#include "defs.h"
#include "fs.h"
#include "buf.h"

#define NBUK 13

uint hash_func(uint i) {
  return i % NBUK;
}

uint get_ticks() {
  acquire(&tickslock);
  uint ret = ticks;
  release(&tickslock);
  return ret;
}

struct {
  struct spinlock big_lock;
  struct spinlock bucket_lock[NBUK];
  struct buf buf[NBUF];
  struct buf head[NBUK];

  // Linked list of all buffers, through prev/next.
  // Sorted by how recently the buffer was used.
  // head.next is most recent, head.prev is least.
} bcache;

void
binit(void)
{
  initlock(&bcache.big_lock, "bcache");
  for (int i = 0; i < NBUK; ++i) {
    initlock(&bcache.bucket_lock[i], "bcache.bucket");
    bcache.head[i].next = bcache.head[i].prev = &bcache.head[i];
  }
  for (int i = 0; i < NBUF; ++i) {
    struct buf *cur_buf = &bcache.buf[i];
    struct buf *cur_head = &bcache.head[hash_func(cur_buf->blockno)];
    cur_buf->timestamp = get_ticks();
    cur_buf->next = cur_head->next;
    cur_buf->prev = cur_head;
    cur_head->next->prev = cur_buf;
    cur_head->next = cur_buf;
    initsleeplock(&cur_buf->lock, "buffer");
  }
}

// Look through buffer cache for block on device dev.
// If not found, allocate a buffer.
// In either case, return locked buffer.
static struct buf*
bget(uint dev, uint blockno)
{
  struct buf *b;

  int id = hash_func(blockno);
  acquire(&bcache.bucket_lock[id]);
  for (b = bcache.head[id].next; b != &bcache.head[id]; b = b->next) {
    if (b->dev == dev && b->blockno == blockno) {
      ++b->refcnt;
      release(&bcache.bucket_lock[id]);
      acquiresleep(&b->lock);
      return b;
    }
  }
  release(&bcache.bucket_lock[id]);

  acquire(&bcache.big_lock);
  acquire(&bcache.bucket_lock[id]);
  for (b = bcache.head[id].next; b != &bcache.head[id]; b = b->next) {
    if (b->dev == dev && b->blockno == blockno) {
      ++b->refcnt;
      release(&bcache.big_lock);
      release(&bcache.bucket_lock[id]);
      acquiresleep(&b->lock);
      return b;
    }
  }
  release(&bcache.bucket_lock[id]);

  uint min_tk = ~0;
  struct buf *to_select = 0;
  int which_bucket = -1;
  for (int i = 0; i < NBUK; ++i) {
    acquire(&bcache.bucket_lock[i]);
    for (b = bcache.head[i].prev; b != &bcache.head[i]; b = b->prev) {
      if (b->refcnt != 0 || b->timestamp >= min_tk)
        continue;
      min_tk = b->timestamp;
      to_select = b;
      which_bucket = i;
    }
    release(&bcache.bucket_lock[i]);
  }


  if (to_select == 0) {
    panic("bget: no buffers");
  }

  acquire(&bcache.bucket_lock[id]);
  if (id != which_bucket) {
    acquire(&bcache.bucket_lock[which_bucket]);
  }
  to_select->dev = dev;
  to_select->blockno = blockno;
  to_select->valid = 0;
  to_select->refcnt = 1;
  b->timestamp = get_ticks();

  to_select->prev->next = to_select->next;
  to_select->next->prev = to_select->prev;

  to_select->next = bcache.head[id].next;
  to_select->prev = &bcache.head[id];
  bcache.head[id].next->prev = to_select;
  bcache.head[id].next = to_select;

  if (id != which_bucket) {
    release(&bcache.bucket_lock[which_bucket]);
  }
  release(&bcache.bucket_lock[id]);

  release(&bcache.big_lock);
  acquiresleep(&to_select->lock);

  return to_select;
}

// Return a locked buf with the contents of the indicated block.
struct buf*
bread(uint dev, uint blockno)
{
  struct buf *b;

  b = bget(dev, blockno);
  if(!b->valid) {
    virtio_disk_rw(b, 0);
    b->valid = 1;
  }
  return b;
}

// Write b's contents to disk.  Must be locked.
void
bwrite(struct buf *b)
{
  if(!holdingsleep(&b->lock))
    panic("bwrite");
  virtio_disk_rw(b, 1);
}

// Release a locked buffer.
// Move to the head of the most-recently-used list.
void
brelse(struct buf *b)
{
  if(!holdingsleep(&b->lock))
    panic("brelse");

  releasesleep(&b->lock);
  int id = hash_func(b->blockno);
  acquire(&bcache.bucket_lock[id]);
  if (--b->refcnt == 0) {
    b->next->prev = b->prev;
    b->prev->next = b->next;
    b->next = bcache.head[id].next;
    b->prev = &bcache.head[id];
    bcache.head[id].next->prev = b;
    bcache.head[id].next = b;
    b->timestamp = get_ticks();
  }
  release(&bcache.bucket_lock[id]);
}

void
bpin(struct buf *b) {
  acquire(&bcache.big_lock);
  b->refcnt++;
  release(&bcache.big_lock);
}

void
bunpin(struct buf *b) {
  acquire(&bcache.big_lock);
  b->refcnt--;
  release(&bcache.big_lock);
}


