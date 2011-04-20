/**
 * \file avb_1722_router.c
 * \brief A packet router for 1722. Allows multiple listeners on multiple streams
 */

#include <xccompat.h>
#include "avb_1722_router.h"
#include "avb_1722_router_table.h"
#include "avb_srp.h"
#include "print.h"
static chanend avb_1722_links[MAX_AVB_1722_ROUTER_LINKS];

void avb_1722_register_routes(chanend link0,
                             chanend link1,
                             chanend link2,
                             chanend link3, 
                             int num_clients) 
{
  if (num_clients >= 1)
    avb_1722_links[0] = link0;
  if (num_clients >= 2)
    avb_1722_links[1] = link1;
  if (num_clients >= 3)
    avb_1722_links[2] = link2;
  if (num_clients >= 4)
    avb_1722_links[3] = link3;  
  return;
}


void send_avb_1722_router_cmd(chanend,
                              unsigned,
                              unsigned,
                              unsigned,
                              unsigned);

int avb_1722_add_stream_mapping(chanend c_tx,
                                unsigned int streamId[2],
                                int link_num,
                                int avb_hash)
{
  unsigned char *s = (unsigned char *) streamId;
  int key0 = 
        (s[3] << 0)  |
        (s[2] << 8)  |
        (s[5] << 16) | 
        (s[4] << 24); 
  int key1 = 
        (s[1] << 0)  |
        (s[0] << 8)  |
        (s[7] << 16) | 
        (s[6] << 24);    

  send_avb_1722_router_cmd(c_tx, 
                           key0, 
                           key1, 
                           avb_1722_links[link_num], 
                           avb_hash);

  return 0;
}


int avb_1722_disconnect_stream_mapping(chanend c_tx,
                                       unsigned int streamId[2])
{
  unsigned char *s = (unsigned char *) streamId;
  int key0 = 
        (s[3] << 0)  |
        (s[2] << 8)  |
        (s[5] << 16) | 
        (s[4] << 24); 
  int key1 = 
        (s[1] << 0)  |
        (s[0] << 8)  |
        (s[7] << 16) | 
        (s[6] << 24);    

  
  send_avb_1722_router_cmd(c_tx,
                           key0,
                           key1,
                           0,
                           0);
  return 0;
}
