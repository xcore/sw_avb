#ifndef __avb_stream_detect_h__
#define __avb_stream_detect_h__
#include <xccompat.h>
#ifdef __XC__
#define NULLABLE ?
#else
#define NULLABLE
#endif

void avb_add_detected_stream(unsigned streamId[2], unsigned vlan,
                             unsigned char NULLABLE addr[6],
                             int addr_offset);


/** Check whether a new incoming AVB stream has been detected.
 *
 *  This function checks whether a new IEEE 1722 stream has been detected.
 *  A new stream will be detected if an 802.1Qat talker advertise packet is
 *  received by the endpoint.
 *
 *  Each new stream seen will be reported once by this function. Internally
 *  the AVB system keeps a history up to the size ``AVB_STREAM_HISTORY_SIZE``
 *  which has a default value of 128 and can be set in ``avb_conf.h``
 *
 *  \param streamId     an array to be filled with the new stream id
 *  \param vlan         a reference param to be filled with the vlan the
 *                      detected stream is on
 *  \returns            a non-zero value if a new stream is detected, zero
 *                      otherwise
 **/
int avb_check_for_new_stream(unsigned streamId[2],
                             REFERENCE_PARAM(unsigned, vlan),
                             unsigned char addr[6]);

#endif
