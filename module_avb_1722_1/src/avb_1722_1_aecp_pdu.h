#ifndef AVB_1722_1_AECPDU_H_
#define AVB_1722_1_AECPDU_H_

#include "avb_1722_1_protocol.h"
#include "avb_1722_1_default_conf.h"

/**
 * 1722.1 AECP command format
 */
typedef struct {
	unsigned char mode_len;
	unsigned char lower_len;
	unsigned char oui[4];
	unsigned char oui_sect_flags[2];
	unsigned char type_code_flags[2];
	unsigned char index0[2];
	unsigned char subaddress[2];
	unsigned char index1[2];
	unsigned char index2[2];
	unsigned char mode_specific_data[1];
} avb_1722_1_aecp_avdecc_msg_t;

/**
 * 1722.1 AECP Address access format
 */
typedef struct {
	unsigned char mode_len;
	unsigned char lower_len;
	unsigned char mode_specific_data[1];
} avb_1722_1_aecp_address_access_t;

/**
 * 1722.1m AECP Legacy AV/C command format
 */
typedef struct {
	unsigned char length[2];
	unsigned char avc_data[512];
} avb_1722_1_aecp_avc_t;

/**
 * 1722.1 AECP Vendor specific command format
 */
typedef struct {
	unsigned char protocol_id[6];
	unsigned char payload_data[1];
} avb_1722_1_aecp_vendor_t;

typedef enum {
	AECP_CMD_AVDECC_MSG_COMMAND = 0,
	AECP_CMD_AVDECC_MSG_RESPONSE = 1,
	AECP_CMD_ADDRESS_ACCESS_COMMAND = 2,
	AECP_CMD_ADDRESS_ACCESS_RESPONSE = 3,
	AECP_CMD_AVC_COMMAND = 4,
	AECP_CMD_AVC_RESPONSE = 5,
	AECP_CMD_VENDOR_UNIQUE_COMMAND = 6,
	AECP_CMD_VENDOR_UNIQUE_RESPONSE = 7,
	AECP_CMD_EXTENDED_COMMAND = 14,
	AECP_CMD_EXTENDED_RESPONSE = 15
} avb_1722_1_aecp_message_type;

typedef enum {
	AECP_STATUS_SUCCESS = 0,
	AECP_STATUS_NOT_IMPLEMENTED = 1
} avb_1722_1_aecp_status_type;

/**
 *  A 1722.1 AECP packet
 *
 * \note all elements 16 bit aligned
 */
typedef struct {
	avb_1722_1_packet_header_t header;
	unsigned char target_guid[8];
	unsigned char controller_guid[8];
	short sequence_id;
	union {
		avb_1722_1_aecp_avdecc_msg_t avdecc;
		avb_1722_1_aecp_address_access_t address;
		avb_1722_1_aecp_avc_t avc;
		avb_1722_1_aecp_vendor_t vendor;
		unsigned char payload[514];
	} data;
} avb_1722_1_aecp_packet_t;


#endif /* AVB_1722_1_AECPDU_H_ */
