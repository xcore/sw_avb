/**
 * \file avb_1722_router.h
 * \brief Definitions for the AVB 1722 packet router
 */

#ifndef _avb_1722_router_h_
#define _avb_1722_router_h_
#include <xccompat.h>
#include "avb_conf.h"

#ifndef MAX_CHANNELS_PER_AVB_STREAM
#define MAX_CHANNELS_PER_AVB_STREAM 16
#endif


#ifndef MAX_AVB_1722_ROUTER_LINKS
#define MAX_AVB_1722_ROUTER_LINKS 4
#endif

#ifndef AVB_1722_ROUTER_BUFFER_SIZE
#define AVB_1722_ROUTER_BUFFER_SIZE 2048
#endif



#ifdef __XC__
void avb_1722_register_routes(chanend link0,
                             chanend link1,
                             chanend link2,
                             chanend link3,
                             int num_clients);
#else
void avb_1722_register_routes(chanend link0,
                             chanend link1,
                             chanend link2,
                             chanend link3,
                             int num_clients);
#endif

#ifdef __XC__
void avb_1722_router(chanend ether_rx,
                    chanend avb_1722_link[],
                    int num_clients);
#else
void avb_1722_router(chanend ether_rx,
                    chanend avb_1722_link[],
                     int num_clients);
#endif

void avb_1722_router_buffer_packets(chanend ether_rx);

void avb_1722_router_send_packets(void);

void avb_1722_router_buffer_packets0(chanend ethernet_rx_svr,
                                    unsigned int buf[],
                                    REFERENCE_PARAM(int,rdIndex),
                                    REFERENCE_PARAM(int,wrIndex));

void avb_1722_router_send_packet(chanend link,
                                unsigned int buf[],
                                int avb_hash,
                                int i);


int avb_1722_check_new_stream(unsigned int streamId[2]);



void avb_1722_enable_stream_forwarding(chanend c_tx,
                                      unsigned int stream_id[2]);

void avb_1722_disable_stream_forwarding(chanend c_tx,
                                       unsigned int stream_id[2]);

void avb_1722_add_stream_mapping(chanend c_tx,
                                unsigned int stream_id[2],
                                int link_num,
                                int avb_hash);

void avb_1722_remove_stream_mapping(chanend c_tx,
                                    unsigned int streamId[2]);

void avb_1722_remove_stream_from_table(chanend c_tx,
                                        unsigned int streamId[2]);

#endif // _avb_1722_router_h_
