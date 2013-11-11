#ifndef __AVB_1722_1_AECP_CONTROLS_H__
#define __AVB_1722_1_AECP_CONTROLS_H__

#include "avb_1722_1_aecp_pdu.h"
#include "xc2compat.h"
#include "avb_api.h"
#include "avb_1722_1_callbacks.h"

unsafe unsigned short process_aem_cmd_getset_control(avb_1722_1_aecp_packet_t *unsafe pkt,
                                                     REFERENCE_PARAM(unsigned char, status),
                                                     unsigned short command_type,
                                                     CLIENT_INTERFACE(avb_1722_1_control_callbacks, i_1722_1_entity));

unsafe void process_aem_cmd_getset_sampling_rate(avb_1722_1_aecp_packet_t *unsafe pkt,
                                          REFERENCE_PARAM(unsigned char, status),
                                          unsigned short command_type,
                                          CLIENT_INTERFACE(avb_interface, i_avb));

unsafe void process_aem_cmd_getset_clock_source(avb_1722_1_aecp_packet_t *unsafe pkt,
                                         REFERENCE_PARAM(unsigned char, status),
                                         unsigned short command_type,
                                         CLIENT_INTERFACE(avb_interface, i_avb));

unsafe void process_aem_cmd_startstop_streaming(avb_1722_1_aecp_packet_t *unsafe pkt,
                                         REFERENCE_PARAM(unsigned char, status),
                                         unsigned short command_type,
                                         CLIENT_INTERFACE(avb_interface, i_avb));

#endif
