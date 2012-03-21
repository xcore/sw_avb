#ifndef AVB_1722_1_AECPDU_H_
#define AVB_1722_1_AECPDU_H_

#include "avb_1722_1_protocol.h"
#include "avb_1722_1_default_conf.h"

#define AEM_MSG_U_FLAG(pkt)			((pkt)->uflag_command_type >> 7)
#define AEM_MSG_COMMAND_TYPE(pkt)	((((pkt)->uflag_command_type & 0x7f) << 8)| \
                                        ((pkt)->command_type))


#define AVB_1722_1_AECP_CD_LENGTH	40

/**
 * 1722.1 AECP AEM command format
 */
typedef struct {
	unsigned char uflag_command_type;
	unsigned char command_type;
	unsigned char payload[512];
} avb_1722_1_aecp_aem_msg_t;

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
	unsigned char avc_length[2];
	unsigned char avc_command_response[512];
} avb_1722_1_aecp_avc_t;

/**
 * 1722.1 AECP Vendor specific command format
 */
typedef struct {
	unsigned char protocol_id[6];
	unsigned char payload_data[1];
} avb_1722_1_aecp_vendor_t;

/**
 *  A 1722.1 AECP packet
 *
 * \note all elements 16 bit aligned
 */
typedef struct {
	avb_1722_1_packet_header_t header;
	unsigned char target_guid[8];
	unsigned char controller_guid[8];
	unsigned char sequence_id[2];
	union {
		avb_1722_1_aecp_aem_msg_t avdecc;
		avb_1722_1_aecp_address_access_t address;
		avb_1722_1_aecp_avc_t avc;
		avb_1722_1_aecp_vendor_t vendor;
		unsigned char payload[514];
	} data;
} avb_1722_1_aecp_packet_t;

#define AVB_1722_1_AECP_PAYLOAD_OFFSET (sizeof(avb_1722_1_packet_header_t) + 18)

typedef enum {
	AECP_CMD_AEM_COMMAND = 0,
	AECP_CMD_AEM_RESPONSE = 1,
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
	AECP_AEM_CMD_LOCK_ENTITY = 0,
	AECP_AEM_CMD_READ_DESCRIPTOR = 1,
	AECP_AEM_CMD_WRITE_DESCRIPTOR = 2,
	AECP_AEM_CMD_ACQUIRE_ENTITY = 3,
	AECP_AEM_CMD_CONTROLLER_AVAILABLE = 4,
	AECP_AEM_CMD_SET_CLOCK_SOURCE = 5,
	AECP_AEM_CMD_GET_CLOCK_SOURCE = 6,
	AECP_AEM_CMD_SET_STREAM_FORMAT = 7, 
	AECP_AEM_CMD_GET_STREAM_FORMAT = 8,
	AECP_AEM_CMD_SET_CONFIGURATION = 9, 
	AECP_AEM_CMD_GET_CONFIGURATION = 10, 
	AECP_AEM_CMD_SET_CONTROL_VALUE = 11,
	AECP_AEM_CMD_GET_CONTROL_VALUE = 12,
	AECP_AEM_CMD_SET_SIGNAL_SELECTOR = 13,
	AECP_AEM_CMD_GET_SIGNAL_SELECTOR = 14,
	AECP_AEM_CMD_SET_MIXER = 15,
	AECP_AEM_CMD_GET_MIXER = 16, 
	AECP_AEM_CMD_SET_MATRIX = 17,
	AECP_AEM_CMD_GET_MATRIX = 18, 
	AECP_AEM_CMD_START_STREAMING = 19,
	AECP_AEM_CMD_STOP_STREAMING = 20, 
	AECP_AEM_CMD_SET_STREAM_INFO = 21, 
	AECP_AEM_CMD_GET_STREAM_INFO = 22, 
	AECP_AEM_CMD_SET_NAME = 23,
	AECP_AEM_CMD_GET_NAME = 24,
	AECP_AEM_CMD_SET_ASSOCIATION_ID = 25,
	AECP_AEM_CMD_GET_ASSOCIATION_ID = 26,
	AECP_AEM_CMD_AUTH_ADD_KEY = 27,
	AECP_AEM_CMD_AUTH_GET_KEY = 28,
	AECP_AEM_CMD_AUTHENTICATE = 29,
	AECP_AEM_CMD_GET_COUNTERS = 30, 
	AECP_AEM_CMD_REBOOT = 31,
	AECP_AEM_CMD_SET_MEDIA_FORMAT = 32,
	AECP_AEM_CMD_GET_MEDIA_FORMAT = 33,
	AECP_AEM_CMD_REGISTER_STATE_NOTIFICATION = 34,
	AECP_AEM_CMD_DEREGISTER_STATE_NOTIFICATION = 35,
	AECP_AEM_CMD_REGISTER_QUERY_NOTIFICATION = 36,
	AECP_AEM_CMD_DEREGISTER_QUERY_NOTIFICATION = 37,
	AECP_AEM_CMD_IDENTIFY_NOTIFICATION = 38,
	AECP_AEM_CMD_STATE_CHANGE_NOTIFICATION = 39,
	AECP_AEM_CMD_INCREMENT_CONTROL_VALUE = 40,
	AECP_AEM_CMD_DECREMENT_CONTROL_VALUE = 41,
	AECP_AEM_CMD_START_OPERATION = 42, 
	AECP_AEM_CMD_ABORT_OPERATION = 43, 
	AECP_AEM_CMD_OPERATION_STATUS = 44,
	AECP_AEM_CMD_AUTH_GET_KEY_COUNT = 45,
	AECP_AEM_CMD_GET_AS_PATH = 46,
	AECP_AEM_CMD_DEAUTHENTICATE = 47,
	AECP_AEM_CMD_AUTH_REVOKE_KEY = 48
} avb_1722_1_aecp_aem_cmd_code;

typedef enum {
	AECP_AEM_STATUS_SUCCESS = 0,
	AECP_AEM_STATUS_NOT_IMPLEMENTED = 1,
	AECP_AEM_STATUS_NO_SUCH_DESCRIPTOR = 2,
	AECP_AEM_STATUS_ENTITY_LOCKED = 3,
	AECP_AEM_STATUS_ENTITY_ACQUIRED = 4,
	AECP_AEM_STATUS_NOT_AUTHORIZED = 5,
	AECP_AEM_STATUS_INSUFFICIENT_PRIVILEDGES = 6,
	AECP_AEM_STATUS_BAD_ARGUMENTS = 7,
	AECP_AEM_STATUS_NO_RESOURCES = 8,
	AECP_AEM_STATUS_IN_PROGRESS = 9
} avb_1722_1_aecp_aem_status_code;

typedef enum {
	AECP_STATUS_SUCCESS = 0,
	AECP_STATUS_NOT_IMPLEMENTED = 1
} avb_1722_1_aecp_status_type;


#endif /* AVB_1722_1_AECPDU_H_ */
