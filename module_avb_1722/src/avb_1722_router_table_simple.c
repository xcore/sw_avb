/**
 * \file avb_1722_router_table_simple.c
 * \brief A simple non-hashed routing table for the 1722 packet router
 */


#include <xs1.h>
#include <print.h>
#include "simple_printf.h"
#include "swlock.h"
#include "string.h"
#include "avb_1722_router_table.h"

 #define AVB_MAX_NUM_SINK_AND_FORWARD_STREAMS 8

typedef struct avb_1722_router_table_entry_t 
{
  int id[2];
  int link;
  int avb_hash;
} avb_1722_router_table_entry_t;

static avb_1722_router_table_entry_t router_table[AVB_MAX_NUM_SINK_AND_FORWARD_STREAMS];
// static hwlock_t table_lock;
static swlock_t table_lock;

#define __hwlock_init() swlock_init(&table_lock)
#define hwlock_release(x) swlock_release(&x)
#define hwlock_acquire(x) swlock_acquire(&x)

void init_avb_1722_router_table_simple()
{
  int i;
  // table_lock = __hwlock_init();
  swlock_init(&table_lock);
  for(i=0;i<AVB_MAX_NUM_SINK_AND_FORWARD_STREAMS;i++) {
    router_table[i].id[0] = 0;
    router_table[i].id[1] = 0;
  }    
}


#define STRINGIFY0(X) #X
#define STRINGIFY(X) STRINGIFY0(X)

int avb_1722_router_table_lookup_simple(int key0,
                                        int key1, 
                                        int *link, 
                                        int *avb_hash,
                                        int *forward) 
{

  if (key0==0 && key1==0) {
    return 0;
  }

  hwlock_acquire(table_lock);      
  
  for(int i=0;i<AVB_MAX_NUM_SINK_AND_FORWARD_STREAMS;i++) {
    __asm__(".xtaloop " STRINGIFY(AVB_MAX_NUM_SINK_AND_FORWARD_STREAMS) "\n");
    if (key0 == router_table[i].id[0] &&
        key1 == router_table[i].id[1]) {
      if ((router_table[i].avb_hash & 0x80000000) == 0x80000000) {
        *forward = 1;
        *avb_hash = router_table[i].avb_hash & ~(0x80000000);
      }
      else
      {
        *avb_hash = router_table[i].avb_hash;
        *forward = 0;
      }
      *link = router_table[i].link;
      hwlock_release(table_lock);
      return 1;
    }

  }
  hwlock_release(table_lock);
  return 0;
}



void avb_1722_router_table_add_entry_simple(int key0,
                                            int key1,
                                            int link,
                                            int avb_hash,
                                            int forward)
{
  hwlock_acquire(table_lock);

  for(int i=0;i<AVB_MAX_NUM_SINK_AND_FORWARD_STREAMS;i++) {
    if (key0 == router_table[i].id[0] &&
        key1 == router_table[i].id[1]) {
      // Found an existing entry with this stream ID, update it
      if (router_table[i].avb_hash & 0x80000000) {
        avb_hash |= 0x80000000;
      }
      
      if (!forward) {
        router_table[i].link = link;
      }
      else
      {
        avb_hash |= 0x80000000;
      }

      router_table[i].avb_hash = avb_hash;
      simple_printf("Updated table entry %x:%x, hash: %x, link: %d, forward: %d\n", router_table[i].id[0], router_table[i].id[1], router_table[i].avb_hash, router_table[i].link, forward);
      hwlock_release(table_lock);
      return;
    }
  }

  // Add a new entry
  for(int i=0;i<AVB_MAX_NUM_SINK_AND_FORWARD_STREAMS;i++) {
    if (router_table[i].id[0] == 0) {
      if (forward) avb_hash |= 0x80000000;
      simple_printf("Added table entry %x:%x, hash: %x, link: %d, forward: %d\n", key0, key1, avb_hash, link, forward);
      router_table[i].link = link;
      router_table[i].avb_hash = avb_hash;
      router_table[i].id[0] = key0;
      router_table[i].id[1] = key1;
      break;
    }
  }
  hwlock_release(table_lock);

  return;
}
