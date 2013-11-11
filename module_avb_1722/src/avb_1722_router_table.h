/**
 * \file avb_1722_router_table.h
 * \brief Definition for the 1722 router table
 */

#ifndef _avb_1722_router_table_h_
#define _avb_1722_router_table_h_
#include <xccompat.h>
#include "avb_conf.h"

#ifndef AVB_1722_ROUTER_TABLE_SIZE
#define AVB_1722_ROUTER_TABLE_SIZE 256
#endif

void init_avb_1722_router_table_hash();

int avb_1722_router_table_lookup_hash(int key0,
                                int key1,
                                REFERENCE_PARAM(int, link),
                                REFERENCE_PARAM(int, avb_hash));

void avb_1722_router_table_add_entry_hash(int key0,
                                          int key1,
                                          int link,
                                          int avb_hash);


void init_avb_1722_router_table_simple();

int avb_1722_router_table_lookup_simple(int key0,
                                        int key1,
                                        REFERENCE_PARAM(int, link),
                                        REFERENCE_PARAM(int, avb_hash),
                                        REFERENCE_PARAM(int, f0rward));

void avb_1722_router_table_add_or_update_entry_simple(int key0,
                                                      int key1,
                                                      int link,
                                                      int avb_hash);

void avb_1722_router_table_add_or_update_forwarding_simple(int key0, int key1, int f0rward);

void avb_1722_router_table_remove_entry_simple(int key0, int key1);


#if AVB_1722_USE_HASHING_ROUTER_TABLE

#define avb_1722_router_table_lookup avb_1722_router_table_lookup_hash
#define avb_1722_router_table_add_entry avb_1722_router_table_add_entry_hash
#define init_avb_1722_router_table init_avb_1722_router_table_hash

#else


#define avb_1722_router_table_lookup avb_1722_router_table_lookup_simple
#define avb_1722_router_table_add_or_update_entry avb_1722_router_table_add_or_update_entry_simple
#define avb_1722_router_table_add_or_update_forwarding avb_1722_router_table_add_or_update_forwarding_simple
#define init_avb_1722_router_table init_avb_1722_router_table_simple
#define avb_1722_router_table_remove_entry avb_1722_router_table_remove_entry_simple

#endif
#endif // _avb_1722_router_h_
