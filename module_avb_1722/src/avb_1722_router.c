/**
 * \file avb_1722_router.c
 * \brief A packet router for 1722. Allows multiple listeners on multiple streams
 */

#include <xccompat.h>
#include "avb_1722_router.h"
#include "avb_1722_router_table.h"
#include "print.h"
#include "simple_printf.h"
#include "ethernet_tx_client.h"
static chanend avb_1722_links[MAX_AVB_1722_ROUTER_LINKS];

#define AVB_1722_ROUTER_UNMAPPED -1

#define DEBUG_1722_ROUTER 1

void avb_1722_register_routes(chanend link0,
                             chanend link1,
                             chanend link2,
                             chanend link3, 
                             int num_clients) {
  if (num_clients >= 1)
    avb_1722_links[0] = link0;
  if (num_clients >= 2)
    avb_1722_links[1] = link1;
  if (num_clients >= 3)
    avb_1722_links[2] = link2;
  if (num_clients >= 4)
    avb_1722_links[3] = link3;  
}


void send_avb_1722_router_cmd(chanend,
                              int,
                              int,
                              int,
                              int,
                              int);

static void keys_from_stream_id(unsigned int stream_id[2], int *key0, int *key1) {
  unsigned char *s = (unsigned char *) stream_id;

  *key0 = 
        (s[3] << 0)  |
        (s[2] << 8)  |
        (s[5] << 16) | 
        (s[4] << 24); 
  *key1 = 
        (s[1] << 0)  |
        (s[0] << 8)  |
        (s[7] << 16) | 
        (s[6] << 24);
}

void avb_1722_enable_stream_forwarding(chanend c_tx,
                                      unsigned int stream_id[2]) {
  int key0, key1;
  keys_from_stream_id(stream_id, &key0, &key1);

  if (DEBUG_1722_ROUTER) {
    simple_printf("1722 router: Enabled forwarding for stream %x%x\n", stream_id[0], stream_id[1]);
  }

  mac_1722_router_enable_forwarding(c_tx, key0, key1);
}

void avb_1722_disable_stream_forwarding(chanend c_tx,
                                       unsigned int stream_id[2]) {
  int key0, key1;
  keys_from_stream_id(stream_id, &key0, &key1);

  if (DEBUG_1722_ROUTER) {
    simple_printf("1722 router: Disabled forwarding for stream %x%x\n", stream_id[0], stream_id[1]);
  }

  mac_1722_router_disable_forwarding(c_tx, key0, key1);
}

void avb_1722_add_stream_mapping(chanend c_tx,
                                unsigned int stream_id[2],
                                int link_num,
                                int avb_hash) {
  int key0, key1;
  keys_from_stream_id(stream_id, &key0, &key1);

  if (DEBUG_1722_ROUTER) {
    simple_printf("1722 router: Enabled map for stream %x%x (link_num:%x, hash:%x)\n", stream_id[0], stream_id[1], link_num, avb_hash);
  }

  mac_1722_update_router(c_tx, key0, key1,
                        link_num != AVB_1722_ROUTER_UNMAPPED ? avb_1722_links[link_num] : AVB_1722_ROUTER_UNMAPPED,
                        avb_hash);

}


void avb_1722_remove_stream_mapping(chanend c_tx,
                                    unsigned int stream_id[2])
{
  int key0, key1;
  keys_from_stream_id(stream_id, &key0, &key1);

  if (DEBUG_1722_ROUTER) {
    simple_printf("1722 router: Disabled map for stream %x%x\n", stream_id[0], stream_id[1]);
  }

  mac_1722_update_router(c_tx, key0, key1, AVB_1722_ROUTER_UNMAPPED, AVB_1722_ROUTER_UNMAPPED);

}
