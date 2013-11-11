/**
 * \file avb_1722_router_table_hash.c
 * \brief Hash table and hash for 1722 packet router
 */

#include <xs1.h>
#include <print.h>
#include "hwlock.h"
#include "string.h"
#include "avb_1722_router_table.h"
typedef struct avb_1722_router_table_entry_t
{
  int id[2];
  int link;
  int avb_hash;
} avb_1722_router_table_entry_t;


static hwlock_t table_lock;

static avb_1722_router_table_entry_t router_table0[AVB_1722_ROUTER_TABLE_SIZE];
static avb_1722_router_table_entry_t router_table1[AVB_1722_ROUTER_TABLE_SIZE];

static avb_1722_router_table_entry_t *router_table = router_table0;
static avb_1722_router_table_entry_t *backup_table  = router_table1;

static int num_entries = 0;
static int router_table_poly[2] = {0xEDB88320, 0xba75fe21};
static int backup_table_poly[2] = {0xEDB88320, 0xba75fe21};
static unsigned int a=1664525;
static unsigned int c=1013904223;




void init_avb_1722_router_table_hash()
{
  int i;
  table_lock = hwlock_alloc();
  num_entries = 0;
  for(i=0;i<AVB_1722_ROUTER_TABLE_SIZE;i++) {
    router_table[i].id[0] = 0;
    router_table[i].id[1] = 0;
    backup_table[i].id[0] = 0;
    backup_table[i].id[1] = 0;
  }
}

static inline int hash(int key0, int key1, int poly)
{
  unsigned int x=0x9226F562;

  //  crc32(x, key0, poly);
  //  crc32(x, key1, poly);
  __asm("crc32 %0, %2, %3":"=r"(x):"0"(x),"r"(key0),"r"(poly));
  __asm("crc32 %0, %2, %3":"=r"(x):"0"(x),"r"(key1),"r"(poly));

  x = x & (AVB_1722_ROUTER_TABLE_SIZE-1);
  return x;
}

int avb_1722_router_table_lookup_hash(int key0,
                                      int key1,
                                      int *link,
                                      int *avb_hash)
{
  unsigned int x;

  if (key0==0 && key1==0)
    return 0;

  hwlock_acquire(table_lock);
  x = hash(key0, key1, router_table_poly[0]);

  if (key0 == router_table[x].id[0] &&
      key1 == router_table[x].id[1]) {

    *link = router_table[x].link;
    *avb_hash = router_table[x].avb_hash;
    hwlock_release(table_lock);
    return 1;
  }

  x = hash(key0, key1, router_table_poly[1]);

  if (key0 == router_table[x].id[0] &&
      key1 == router_table[x].id[1]) {
    *link = router_table[x].link;
    *avb_hash = router_table[x].avb_hash;
    hwlock_release(table_lock);
    return 1;
  }
  hwlock_release(table_lock);
  return 0;
}





static int contains_different_entry(int index, int key[2])
{
  int empty =
    router_table[index].id[0] == 0
    &&
    router_table[index].id[1] == 0;

  int different =
    router_table[index].id[0] != key[0]
    ||
    router_table[index].id[1] != key[1];

  return (!empty && different);
}

static int insert(int key0, int key1, int link, int avb_hash)
{
  int count = 0;
  int conflict = 0;
  int hashtype = 0;
  int curkey[2];
  curkey[0] = key0;
  curkey[1] = key1;
  do {
    int index = hash(curkey[0], curkey[1], backup_table_poly[hashtype]);
    if (!contains_different_entry(index, curkey)) {
      backup_table[index].id[0] = curkey[0];
      backup_table[index].id[1] = curkey[1];
      backup_table[index].link = link;
      backup_table[index].avb_hash = avb_hash;
      conflict = 0;
    }
    else {
      int new_curkey[2];
      conflict=1;
      if (count==0) {
        hashtype = 1-hashtype;
      }
      else {
        new_curkey[0] = backup_table[index].id[0];
        new_curkey[1] = backup_table[index].id[1];
        backup_table[index].id[0] = curkey[0];
        backup_table[index].id[1] = curkey[1];
        backup_table[index].link = link;
        backup_table[index].avb_hash = avb_hash;

        curkey[0] = new_curkey[0];
        curkey[1] = new_curkey[1];
        hashtype = 1-hashtype;
      }
    }

    count++;
  }
  while (conflict && count < num_entries+10);

  if (!conflict)
    num_entries++;

  return !conflict;
}

static void refill_backup_table() {
  int i;
  int success;
  // clear the backup table
  num_entries = 0;
  for(i=0;i<AVB_1722_ROUTER_TABLE_SIZE;i++) {
    backup_table[i].id[0] = 0;
    backup_table[i].id[1] = 0;
  }

  do {
    // change the poly
    backup_table_poly[0] = a * backup_table_poly[0] + c;
    backup_table_poly[1] = a * backup_table_poly[1] + c;
    success = 1;
    for(i=0;success && i<AVB_1722_ROUTER_TABLE_SIZE;i++) {
      if (router_table[i].id[0] != 0 ||
          router_table[i].id[1] != 0) {
        success = insert(router_table[i].id[0],
                         router_table[i].id[1],
                         router_table[i].link,
                         router_table[i].avb_hash);
          }
    }
  } while (!success);
}

void avb_1722_router_table_add_entry_hash(int key0,
                                          int key1,
                                          int link,
                                          int avb_hash)
{
  int success = 0;

  while (!success) {
    success = insert(key0, key1, link, avb_hash);

    if (success) {
      avb_1722_router_table_entry_t *old_table = router_table;
      // flip the tables
      hwlock_acquire(table_lock);
      router_table = backup_table;
      router_table_poly[0] = backup_table_poly[0];
      router_table_poly[1] = backup_table_poly[1];
      hwlock_release(table_lock);
      backup_table = old_table;
      // make sure both tables contain the same
      memcpy(backup_table, router_table, sizeof(router_table0));
    }
    else {
      // refill table with a different hash and try again
      refill_backup_table();
    }
  }
}

