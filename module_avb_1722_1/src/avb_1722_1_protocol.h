#ifndef __AVB_1722_1_PROTOCOL_H__
#define __AVB_1722_1_PROTOCOL_H__

#include <xclib.h>

#define MAX_AVB_1722_1_PDU_SIZE (40)


/**
 * The general header for a 1722 packet
 */
typedef struct avb_1722_1_packet_header_t {
  unsigned char cd_subtype;
  unsigned char sv_avb_version_msg_type;
  unsigned char valid_time_data_length_hi;
  unsigned char data_length_lo;

} avb_1722_1_packet_header_t;

/**
 *  A 1722.1 Simple Discovery Protocol packet
 *
 *  \note all elements 16 bit aligned
 */
typedef struct {
	avb_1722_1_packet_header_t header;
	char entity_guid[8];
	char vendor_id[4];
	char model_id[4];
	char entity_capabilities[4];
	short talker_stream_sources;
	short talker_capabilities;
	short listener_stream_sinks;
	short listener_capabilites;
	char controller_capabilities[4];
	char boot_id[4];
	char as_grandmaster_id[8];
	char reserved[4];
} avb_1722_1_sdp_packet_t;

/**
 *  A 1722.1 Simple Connection Management packet
 *
 * \note all elements 16 bit aligned
 */
typedef struct {
	avb_1722_1_packet_header_t header;
	char stream_id[8];
	char controller_guid[8];
	char talker_guid[8];
	char listener_guid[8];
	short talker_unique_id;
	short listener_unique_id;
	char dest_mac[6];
	short connection_count;
	short sequence_id;
	short flags;
} avb_1722_1_scm_packet_t;

/**
 * 1722.1 SEC AVDECC command format
 */
typedef struct {
	char mode_len;
	char lower_len;
	char mode_specific_data[1];
} avb_1722_1_sec_avdecc_msg_t;

/**
 * 1722.1 SEC Address access format
 */
typedef struct {
	char mode_len;
	char lower_len;
	char mode_specific_data[1];
} avb_1722_1_sec_address_access_t;

/**
 * 1722.1m SEC Legacy AV/C command format
 */
typedef struct {
	char length[2];
	char avc_data[512];
} avb_1722_1_sec_avc_t;

/**
 * 1722.1 SEC Vendor specific command format
 */
typedef struct {
	char protocol_id[6];
	char payload_data[1];
} avb_1722_1_sec_vendor_t;

/**
 *  A 1722.1 Simple Enumeration and Control packet
 *
 * \note all elements 16 bit aligned
 */
typedef struct {
	avb_1722_1_packet_header_t header;
	char target_guid[8];
	char controller_guid[8];
	short sequence_id;
	union {
		avb_1722_1_sec_avdecc_msg_t avdecc;
		avb_1722_1_sec_address_access_t address;
		avb_1722_1_sec_avc_t avc;
		avb_1722_1_sec_vendor_t vendor;
		char payload[514];
	} data;
} avb_1722_1_sec_packet_t;

typedef union {
	avb_1722_1_sdp_packet_t sdp;
	avb_1722_1_scm_packet_t scm;
	avb_1722_1_sec_packet_t sem;
} avb_1722_1_packet_t;

#define DEFAULT_1722_1_CD_FLAG (1)
#define DEFAULT_1722_1_AVB_VERSION (0x0)

#define DEFAULT_1722_1_SDP_SUBTYPE (0x7a)
#define DEFAULT_1722_1_SEC_SUBTYPE (0x7b)
#define DEFAULT_1722_1_SCM_SUBTYPE (0x7c)

#define GET_1722_1_CD_FLAG(pkt) ((pkt)->cd_subtype >> 7)
#define GET_1722_1_SUBTYPE(pkt) ((pkt)->cd_subtype & 0x7f)
#define GET_1722_1_SV(pkt) ((pkt)->sv_avb_version_msg_type >> 7)
#define GET_1722_1_AVB_VERSION(pkt) (((pkt)->sv_avb_version_msg_type & 0x70) >> 4)
#define GET_1722_1_MSG_TYPE(pkt) ((pkt)->sv_avb_version_msg_type & 0x0f)
#define GET_1722_1_VALID_TIME(pkt) ((pkt)->valid_time_data_length_hi & 0xf8 >> 3)
#define GET_1722_1_DATALENGTH(pkt) \
   ((((pkt)->valid_time_data_length_hi & 0x7) << 8) + \
          (((pkt)->data_length_lo) << 8))

#define SET_BITS(p, lo, hi, val) \
  do { \
    *(p) = (*(p) & (~(((1<<(hi-lo+1))-1)<<lo))) | ((val) << lo);        \
  } while(0)

#define SET_1722_1_CD_FLAG(pkt, val) SET_BITS(&pkt->cd_subtype, 7, 7, val)
#define SET_1722_1_SUBTYPE(pkt, val) SET_BITS(&pkt->cd_subtype, 0, 6, val)
#define SET_1722_1_SV(pkt, val)                               \
    SET_BITS(&pkt->sv_avb_version_msg_type, 7, 7, val)

#define SET_1722_1_AVB_VERSION(pkt, val)                      \
    SET_BITS(&pkt->sv_avb_version_msg_type,4,6,val)

#define SET_1722_1_MSG_TYPE(pkt, val)                         \
    SET_BITS(&pkt->sv_avb_version_msg_type,0,3, val)

#define SET_1722_1_VALID_TIME(pkt, val)                                  \
    SET_BITS(&pkt->valid_time_data_length_hi, 3, 7, val)

#define SET_1722_1_DATALENGTH(pkt, val)               \
  do  \
   { \
     SET_BITS(&pkt->valid_time_data_length_hi, 0, 2, (val) >> 8); \
     SET_BITS(&pkt->data_length_lo, 0, 7, (val) & 0xff); \
   }while(0)

#define SET_WORD_CONST(member, data) \
	do { \
		member[0] = ((data >> 24) & 0xff); \
		member[1] = ((data >> 16) & 0xff); \
		member[2] = ((data >> 8 ) & 0xff); \
		member[3] = ((data >> 0 ) & 0xff); \
	} while(0);

#define SET_LONG_WORD(member, data) \
	do { \
		member[0] = data.c[7]; \
		member[1] = data.c[6]; \
		member[2] = data.c[5]; \
		member[3] = data.c[4]; \
		member[4] = data.c[3]; \
		member[5] = data.c[2]; \
		member[6] = data.c[1]; \
		member[7] = data.c[0]; \
	} while(0);

#define GET_LONG_WORD(data, member) \
	do { \
		data.c[7] = member[0]; \
		data.c[6] = member[1]; \
		data.c[5] = member[2]; \
		data.c[4] = member[3]; \
		data.c[3] = member[4]; \
		data.c[2] = member[5]; \
		data.c[1] = member[6]; \
		data.c[0] = member[7]; \
	} while (0);

#define COMPARE_WORD(member, data) \
		((member[0] == (data & 0xffff)) && (member[1] == (data >> 16)))

typedef enum {
	ENTITY_AVAILABLE = 0,
	ENTITY_DEPARTING = 1,
	ENTITY_DISCOVER = 2
} avb_1722_1_sdp_message_type;

#define AVB_1722_1_SDP_ENTITY_CAPABILITES_17221_IP         		(0x00000001)
#define AVB_1722_1_SDP_ENTITY_CAPABILITES_ZERO_CONF        		(0x00000002)
#define AVB_1722_1_SDP_ENTITY_CAPABILITES_BRIDGED_ENTITY   		(0x00000004)
#define AVB_1722_1_SDP_ENTITY_CAPABILITES_17221_CONTROL    		(0x00000008)
#define AVB_1722_1_SDP_ENTITY_CAPABILITES_LEGACY_AVB       		(0x00000010)

#define AVB_1722_1_SDP_TALKER_CAPABILITES_IMPLEMENTED      		(0x0001)
#define AVB_1722_1_SDP_TALKER_CAPABILITES_AUDIO_SOURCE     		(0x4000)
#define AVB_1722_1_SDP_TALKER_CAPABILITES_VIDEO_SOURCE     		(0x8000)

#define AVB_1722_1_SDP_LISTENER_CAPABILITES_IMPLEMENTED    		(0x0001)
#define AVB_1722_1_SDP_LISTENER_CAPABILITES_AUDIO_SOURCE   		(0x4000)
#define AVB_1722_1_SDP_LISTENER_CAPABILITES_VIDEO_SOURCE  	 	(0x8000)

#define AVB_1722_1_SDP_CONTROLLER_CAPABILITES_IMPLEMENTED  		(0x0001)
#define AVB_1722_1_SDP_CONTROLLER_CAPABILITES_LAYER3_PROXY 		(0x0002)


typedef enum {
	SCM_CMD_CONNECT_TX_COMMAND = 0,
	SCM_CMD_CONNECT_TX_RESPONSE	= 1,
	SCM_CMD_DISCONNECT_TX_COMMAND = 2,
	SCM_CMD_DISCONNECT_TX_RESPONSE = 3,
	SCM_CMD_GET_TX_STATE_COMMAND = 4,
	SCM_CMD_GET_TX_STATE_RESPONSE = 5,
	SCM_CMD_CONNECT_RX_COMMAND	= 6,
	SCM_CMD_CONNECT_RX_RESPONSE	= 7,
	SCM_CMD_DISCONNECT_RX_COMMAND = 8,
	SCM_CMD_DISCONNECT_RX_RESPONSE = 9,
	SCM_CMD_GET_RX_STATE_COMMAND = 10,
	SCM_CMD_GET_RX_STATE_RESPONSE = 11,
	SCM_CMD_GET_TX_CONNECTION_COMMAND = 12,
	SCM_CMD_GET_TX_CONNECTION_RESPONSE = 13
} avb_1722_1_scm_message_type;

typedef enum {
	SCM_STATUS_SUCCESS = 0,
	SCM_STATUS_LISTENER_UNKNOWN_ID = 1,
	SCM_STATUS_TALKER_UNKNOWN_ID = 2,
	SCM_STATUS_TALKER_DEST_MAC_FAIL = 3,
	SCM_STATUS_TALKER_NO_STREAM_INDEX = 4,
	SCM_STATUS_TALKER_NO_BANDWIDTH = 5,
	SCM_STATUS_TALKER_EXCLUSIVE = 6,
	SCM_STATUS_LISTENER_TALKER_TIMEOUT = 7,
	SCM_STATUS_LISTENER_EXCLUSIVE = 8,
	SCM_STATUS_STATE_UNAVAILABLE = 9,
	SCM_STATUS_NOT_CONNECTED = 10,
	SCM_STATUS_NO_SUCH_CONNECTION = 11,
	SCM_STATUS_COULD_NOT_SEND_MESSAGE = 12,
	SCM_STATUS_NOT_SUPPORTED = 31
} avb_1722_1_scm_status_type;

typedef enum {
	SEC_CMD_AVDECC_MSG_COMMAND = 0,
	SEC_CMD_AVDECC_MSG_RESPONSE = 1,
	SEC_CMD_ADDRESS_ACCESS_COMMAND = 2,
	SEC_CMD_ADDRESS_ACCESS_RESPONSE = 3,
	SEC_CMD_AVC_COMMAND = 4,
	SEC_CMD_AVC_RESPONSE = 5,
	SEC_CMD_VENDOR_UNIQUE_COMMAND = 6,
	SEC_CMD_VENDOR_UNIQUE_RESPONSE = 7,
	SEC_CMD_EXTENDED_COMMAND = 14,
	SEC_CMD_EXTENDED_RESPONSE = 15
} avb_1722_1_sec_message_type;

typedef enum {
	SEC_STATUS_SUCCESS = 0,
	SEC_STATUS_NOT_IMPLEMENTED = 1
} avb_1722_1_sec_status_type;

#define AVB_1722_1_SCM_FLAGS_CLASS_B							(0x0001)
#define AVB_1722_1_SCM_FLAGS_FAST_CONNECT						(0x0002)
#define AVB_1722_1_SCM_FLAGS_SAVED_STATE						(0x0004)

#endif // __AVB_1722_1_PROTOCOL_H__
