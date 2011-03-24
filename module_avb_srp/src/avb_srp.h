#ifndef _avb_srp_h_
#define _avb_srp_h_
#include <xccompat.h>
#include "avb_1722_talker.h"
#include "avb_mrp.h"
#include "avb_control_types.h"

#define AVB_SRP_ETHERTYPE (0x22ea) 

#define AVB_SRP_MACADDR { 0x01, 0x80, 0xc2, 0x00, 0x00, 0xe }

#define AVB_SRP_LEGACY_MACADDR { 0x01, 0x00, 0x5e, 0x0, 1, 129 }
//#define AVB_SRP_MACADDR { 0xff, 0xff, 0xff, 0xff, 0xff, 0xff }

#define MAX_AVB_SRP_PDU_SIZE (64)





/** Get the stream id of a failed reservation. 
 *
 *  This should be called after getting ``AVB_SRP_ROUTE_FAILED``
 *  from avb_srp_process_packet().
 *
 *  \param streamId the 64 bit stream id array to fill with the failed stream
 *
 **/
void avb_srp_get_failed_stream(unsigned int streamId[2]);


#ifndef __XC__
typedef struct srp_stream_state {
  union {
    mrp_attribute_state talker; 
    mrp_attribute_state listener;
  } u;
} srp_stream_state;



/* The following functions are called from avb_mrp.c */
void avb_srp_process_talker(int attribute_type,
                            char *fv, 
                            int num);

void avb_srp_process_talker_failed(char *fv, 
                                     int num);

void avb_srp_process_listener(char *fv, 
                             int num,
                             int four_packed_event);

void avb_srp_process_domain(char *fv, 
                            int num);

int avb_srp_compare_talker_attributes(mrp_attribute_state *a,
                                      mrp_attribute_state *b);

int avb_srp_compare_listener_attributes(mrp_attribute_state *a,
                                        mrp_attribute_state *b);

int avb_srp_merge_message(char *buf,
                          mrp_attribute_state *st,
                          int vector);

int avb_srp_match_talker_advertise(mrp_attribute_state *attr,
                                   char *msg,
                                   int i);
int avb_srp_match_talker_failed(mrp_attribute_state *attr,
                                char *msg,
                                int i);
int avb_srp_match_listener(mrp_attribute_state *attr,
                           char *msg,
                           int i,
                           int four_packed_event);

int avb_srp_match_domain(mrp_attribute_state *attr,
                         char *msg,
                         int i);
#endif



#endif // _avb_srp_h_
