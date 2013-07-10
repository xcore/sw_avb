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
  int f0rward;
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
                                        int *f0rward) 
{

  if (key0==0 && key1==0) {
    return 0;
  }

  hwlock_acquire(table_lock);      
  
  for(int i=0;i<AVB_MAX_NUM_SINK_AND_FORWARD_STREAMS;i++) {
    __asm__(".xtaloop " STRINGIFY(AVB_MAX_NUM_SINK_AND_FORWARD_STREAMS) "\n");
    if (key0 == router_table[i].id[0] &&
        key1 == router_table[i].id[1]) {

      *avb_hash = router_table[i].avb_hash;
      *f0rward = router_table[i].f0rward;
      *link = router_table[i].link;
      hwlock_release(table_lock);
      return 1;
    }

  }
  hwlock_release(table_lock);
  return 0;
}

void avb_1722_router_table_add_or_update_forwarding_simple(int key0, int key1, int f0rward)
{
  hwlock_acquire(table_lock);

  for(int i=0;i<AVB_MAX_NUM_SINK_AND_FORWARD_STREAMS;i++) {
    if (key0 == router_table[i].id[0] &&
        key1 == router_table[i].id[1]) {
      
      router_table[i].f0rward = f0rward;

      hwlock_release(table_lock);
      return;
    }
  }

  // Add a new entry
  for(int i=0;i<AVB_MAX_NUM_SINK_AND_FORWARD_STREAMS;i++) {
    if (router_table[i].id[0] == 0) {
      router_table[i].f0rward = f0rward;
      router_table[i].link = -1;
      router_table[i].avb_hash = -1;
      router_table[i].id[0] = key0;
      router_table[i].id[1] = key1;
      break;
    }
  }

  hwlock_release(table_lock);  
}

void avb_1722_router_table_add_or_update_entry_simple(int key0,
                                                      int key1,
                                                      int link,
                                                      int avb_hash)
{
  hwlock_acquire(table_lock);

  for(int i=0;i<AVB_MAX_NUM_SINK_AND_FORWARD_STREAMS;i++) {
    if (key0 == router_table[i].id[0] &&
        key1 == router_table[i].id[1]) {
      // Found an existing entry with this stream ID, update it
      
      router_table[i].link = link;
      router_table[i].avb_hash = avb_hash;

      hwlock_release(table_lock);
      return;
    }
  }

  // Add a new entry
  for(int i=0;i<AVB_MAX_NUM_SINK_AND_FORWARD_STREAMS;i++) {
    if (router_table[i].id[0] == 0) {
      router_table[i].f0rward = 0;
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

void avb_1722_router_table_remove_entry_simple(int key0, int key1) {
  hwlock_acquire(table_lock);

  for(int i=0;i<AVB_MAX_NUM_SINK_AND_FORWARD_STREAMS;i++) {
    if (key0 == router_table[i].id[0] &&
        key1 == router_table[i].id[1]) {
      
      router_table[i].id[0] = 0;
      router_table[i].id[1] = 0;

      // FIXME: Zero the whole entry!

      hwlock_release(table_lock);
      return;
    }
  }
  hwlock_release(table_lock);
}