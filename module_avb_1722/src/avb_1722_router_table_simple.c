/**
 * \file avb_1722_router_table_simple.c
 * \brief A simple non-hashed routing table for the 1722 packet router
 */


#include <xs1.h>
#include <print.h>
#include "hwlock.h"
#include "string.h"
#include "avb_1722_router_table.h"
#include "simple_printf.h"

typedef struct avb_1722_router_table_entry_t 
{
  int id[2];
  int link;
  int sink_local_id;
} avb_1722_router_table_entry_t;

static avb_1722_router_table_entry_t router_table[AVB_NUM_SINKS];
static hwlock_t table_lock;

void init_avb_1722_router_table_simple()
{
  int i;
  table_lock = __hwlock_init();
  for(i=0;i<AVB_NUM_SINKS;i++) {
    router_table[i].id[0] = 0;
    router_table[i].id[1] = 0;
  }    
}


#define STRINGIFY0(X) #X
#define STRINGIFY(X) STRINGIFY0(X)

int avb_1722_router_table_lookup_simple(int key0,
                                        int key1, 
                                        unsigned int *link, 
                                        unsigned int *sink_local_id)
{

  if (key0==0 && key1==0)
    return 0;
  __hwlock_acquire(table_lock);      
  for(int i=0;i<AVB_NUM_SINKS;i++) {
    __asm__(".xtaloop " STRINGIFY(AVB_NUM_SINKS) "\n");
    if (key0 == router_table[i].id[0] &&
        key1 == router_table[i].id[1]) {
      *sink_local_id = router_table[i].sink_local_id;
      *link = router_table[i].link;
      __hwlock_release(table_lock);
      return 1;
    }
  }
  __hwlock_release(table_lock);
  return 0;
}



void avb_1722_router_table_add_entry_simple(int key0,
                                            int key1,
                                            int link,
                                            int sink_num,
                                            int sink_local_id)
{
  __hwlock_acquire(table_lock);
  router_table[sink_num].id[0] = key0;
  router_table[sink_num].id[1] = key1;
  router_table[sink_num].link = link;
  router_table[sink_num].sink_local_id = sink_local_id;
#ifdef AVB_1722_DEBUG_ROUTER_TABLE
  simple_printf("avb_1722_router_table_add_entry_simple called for sink_num %d, sink_local_id %d, key0 0x%x, key1 0x%x, link 0x%x\n"
          ,sink_num, sink_local_id, key0, key1, link);
#endif
  __hwlock_release(table_lock);

  return;
}
