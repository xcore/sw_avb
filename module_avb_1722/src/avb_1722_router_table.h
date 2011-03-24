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
                                REFERENCE_PARAM(unsigned int, link), 
                                REFERENCE_PARAM(unsigned int, avb_hash));

void avb_1722_router_table_add_entry_hash(int key0,
                                          int key1,
                                          int link,
                                          int avb_hash);


void init_avb_1722_router_table_simple();

int avb_1722_router_table_lookup_simple(int key0,
                                        int key1, 
                                        REFERENCE_PARAM(unsigned int, link), 
                                        REFERENCE_PARAM(unsigned int, avb_hash));

void avb_1722_router_table_add_entry_simple(int key0,
                                            int key1,
                                            int link,
                                            int avb_hash);



#if AVB_1722_USE_HASHING_ROUTER_TABLE

#define avb_1722_router_table_lookup avb_1722_router_table_lookup_hash
#define avb_1722_router_table_add_entry avb_1722_router_table_add_entry_hash
#define init_avb_1722_router_table init_avb_1722_router_table_hash
 
#else


#define avb_1722_router_table_lookup avb_1722_router_table_lookup_simple
#define avb_1722_router_table_add_entry avb_1722_router_table_add_entry_simple
#define init_avb_1722_router_table init_avb_1722_router_table_simple
 

#endif
#endif // _avb_1722_router_h_
