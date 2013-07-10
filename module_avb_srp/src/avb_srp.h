#ifndef _avb_srp_h_
#define _avb_srp_h_
#include <xccompat.h>
#include "xc2compat.h"
#include "avb_srp_pdu.h"
#include "avb_stream_detect.h"
#include "avb_1722_talker.h"
#include "avb_mrp.h"
#include "avb_control_types.h"
#include "avb_stream.h"

#define AVB_SRP_ETHERTYPE (0x22ea) 

#define AVB_SRP_MACADDR { 0x01, 0x80, 0xc2, 0x00, 0x00, 0xe }
#define AVB_SRP_LEGACY_MACADDR { 0x01, 0x00, 0x5e, 0x0, 1, 129 }

#define MAX_AVB_SRP_PDU_SIZE (64)

void avb_match_and_join_leave(mrp_attribute_state *unsafe attr, int join);


int avb_srp_match_stream_id(unsigned streamId[2], avb_srp_info_t **stream);

int avb_add_detected_stream(srp_talker_first_value *fv,
                            unsigned streamId[2],
                             int addr_offset,
                             avb_srp_info_t **stream);


/** Get the stream id of a failed reservation. 
 *
 *  This should be called after getting ``AVB_SRP_ROUTE_FAILED``
 *  from avb_srp_process_packet().
 *
 *  \param streamId the 64 bit stream id array to fill with the failed stream
 *
 **/
void avb_srp_get_failed_stream(unsigned int streamId[2]);


typedef struct srp_stream_state {
  union {
    mrp_attribute_state talker; 
    mrp_attribute_state listener;
  } u;
} srp_stream_state;



/* The following functions are called from avb_mrp.c */
int avb_srp_process_attribute(int attribute_type,
                            char *fv, 
                            int num,
                            avb_srp_info_t **stream);

int avb_srp_compare_talker_attributes(mrp_attribute_state *a,
                                      mrp_attribute_state *b);

int avb_srp_compare_listener_attributes(mrp_attribute_state *a,
                                        mrp_attribute_state *b);

int avb_srp_encode_message(char *buf,
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

void avb_srp_map_leave(mrp_attribute_state *attr);

//!@{
//! \name Indications from the MRP state machine
void avb_srp_listener_join_ind(mrp_attribute_state *attr, int new, int four_packed_event);
void avb_srp_listener_leave_ind(mrp_attribute_state *attr, int four_packed_event);

void avb_srp_talker_join_ind(mrp_attribute_state *attr, int new);
void avb_srp_talker_leave_ind(mrp_attribute_state *attr);

void avb_srp_domain_join_ind(mrp_attribute_state *attr, int new);
void avb_srp_domain_leave_ind(mrp_attribute_state *attr);
//!@}



#endif // _avb_srp_h_
