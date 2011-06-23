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
 *  A 1722.1 AVDECC Discovery Protocol packet
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
	char available_index[4];
	char as_grandmaster_id[8];
	char default_audio_format[4];
	char default_video_format[4];
	char association_id[8];
} avb_1722_1_adp_packet_t;

/**
 *  A 1722.1 AVDECC Connection Management packet
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
	char default_format[4];
} avb_1722_1_acmp_packet_t;

/**
 * 1722.1 SEC AVDECC command format
 */
typedef struct {
	char mode_len;
	char lower_len;
	char mode_specific_data[1];
} avb_1722_1_aecp_avdecc_msg_t;

/**
 * 1722.1 SEC Address access format
 */
typedef struct {
	char mode_len;
	char lower_len;
	char mode_specific_data[1];
} avb_1722_1_aecp_address_access_t;

/**
 * 1722.1m SEC Legacy AV/C command format
 */
typedef struct {
	char length[2];
	char avc_data[512];
} avb_1722_1_aecp_avc_t;

/**
 * 1722.1 SEC Vendor specific command format
 */
typedef struct {
	char protocol_id[6];
	char payload_data[1];
} avb_1722_1_aecp_vendor_t;

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
		avb_1722_1_aecp_avdecc_msg_t avdecc;
		avb_1722_1_aecp_address_access_t address;
		avb_1722_1_aecp_avc_t avc;
		avb_1722_1_aecp_vendor_t vendor;
		char payload[514];
	} data;
} avb_1722_1_aecp_packet_t;

typedef union {
	avb_1722_1_adp_packet_t sdp;
	avb_1722_1_acmp_packet_t scm;
	avb_1722_1_aecp_packet_t sem;
} avb_1722_1_packet_t;

#define DEFAULT_1722_1_CD_FLAG (1)
#define DEFAULT_1722_1_AVB_VERSION (0x0)

#define DEFAULT_1722_1_ADP_SUBTYPE 	(0x7a)
#define DEFAULT_1722_1_AECP_SUBTYPE (0x7b)
#define DEFAULT_1722_1_ACMP_SUBTYPE (0x7c)

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
		member[0] = (((data) >> 24) & 0xff); \
		member[1] = (((data) >> 16) & 0xff); \
		member[2] = (((data) >> 8 ) & 0xff); \
		member[3] = (((data) >> 0 ) & 0xff); \
	} while(0);

#define GET_WORD(data, member) \
	do { \
		data = (member[0] << 24) + \
		       (member[1] << 16) + \
		       (member[2] << 8) + \
		       (member[3] << 0);\
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

#define AVB_1722_1_ADP_ENTITY_CAPABILITIES_AVDECC_IP         			(0x00000001)
#define AVB_1722_1_ADP_ENTITY_CAPABILITIES_ZERO_CONF        			(0x00000002)
#define AVB_1722_1_ADP_ENTITY_CAPABILITIES_BRIDGED_ENTITY   			(0x00000004)
#define AVB_1722_1_ADP_ENTITY_CAPABILITIES_AVDECC_CONTROL    			(0x00000008)
#define AVB_1722_1_ADP_ENTITY_CAPABILITIES_LEGACY_AVB       			(0x00000010)
#define AVB_1722_1_ADP_ENTITY_CAPIBILITIES_ASSOCIATION_ID_SUPPORTED		(0x00000020)
#define AVB_1722_1_ADP_ENTITY_CAPIBILITIES_ASSOCIATION_ID_VALID			(0x00000040)

#define AVB_1722_1_ADP_TALKER_CAPABILITES_IMPLEMENTED      				(0x0001)
#define AVB_1722_1_ADP_TALKER_CAPABILITES_AUDIO_SOURCE     				(0x4000)
#define AVB_1722_1_ADP_TALKER_CAPABILITES_VIDEO_SOURCE     				(0x8000)

#define AVB_1722_1_ADP_LISTENER_CAPABILITES_IMPLEMENTED    				(0x0001)
#define AVB_1722_1_ADP_LISTENER_CAPABILITES_AUDIO_SOURCE   				(0x4000)
#define AVB_1722_1_ADP_LISTENER_CAPABILITES_VIDEO_SOURCE  	 			(0x8000)

#define AVB_1722_1_ADP_CONTROLLER_CAPABILITES_IMPLEMENTED  				(0x0001)
#define AVB_1722_1_ADP_CONTROLLER_CAPABILITES_LAYER3_PROXY 				(0x0002)

#define AVB_1722_1_ADP_DEFAULT_AUDIO_FORMAT_MONO						(0x00000001)
#define AVB_1722_1_ADP_DEFAULT_AUDIO_FORMAT_2_CH						(0x00000002)
#define AVB_1722_1_ADP_DEFAULT_AUDIO_FORMAT_3_CH						(0x00000004)
#define AVB_1722_1_ADP_DEFAULT_AUDIO_FORMAT_4_CH						(0x00000008)
#define AVB_1722_1_ADP_DEFAULT_AUDIO_FORMAT_5_CH						(0x00000010)
#define AVB_1722_1_ADP_DEFAULT_AUDIO_FORMAT_6_CH						(0x00000020)
#define AVB_1722_1_ADP_DEFAULT_AUDIO_FORMAT_7_CH						(0x00000040)
#define AVB_1722_1_ADP_DEFAULT_AUDIO_FORMAT_8_CH						(0x00000080)
#define AVB_1722_1_ADP_DEFAULT_AUDIO_FORMAT_10_CH						(0x00000100)
#define AVB_1722_1_ADP_DEFAULT_AUDIO_FORMAT_12_CH						(0x00000200)
#define AVB_1722_1_ADP_DEFAULT_AUDIO_FORMAT_14_CH						(0x00000400)
#define AVB_1722_1_ADP_DEFAULT_AUDIO_FORMAT_16_CH						(0x00000800)
#define AVB_1722_1_ADP_DEFAULT_AUDIO_FORMAT_18_CH						(0x00001000)
#define AVB_1722_1_ADP_DEFAULT_AUDIO_FORMAT_20_CH						(0x00002000)
#define AVB_1722_1_ADP_DEFAULT_AUDIO_FORMAT_22_CH						(0x00004000)
#define AVB_1722_1_ADP_DEFAULT_AUDIO_FORMAT_24_CH						(0x00008000)
#define AVB_1722_1_ADP_DEFAULT_AUDIO_FORMAT_FLOAT						(0x00010000)
#define AVB_1722_1_ADP_DEFAULT_AUDIO_FORMAT_SAF							(0x00020000)
#define AVB_1722_1_ADP_DEFAULT_AUDIO_FORMAT_MAX_STREAMS_4				(0x00100000)
#define AVB_1722_1_ADP_DEFAULT_AUDIO_FORMAT_MAX_STREAMS_8				(0x00200000)
#define AVB_1722_1_ADP_DEFAULT_AUDIO_FORMAT_MAX_STREAMS_16				(0x00400000)
#define AVB_1722_1_ADP_DEFAULT_AUDIO_FORMAT_44K1						(0x04000000)
#define AVB_1722_1_ADP_DEFAULT_AUDIO_FORMAT_48K							(0x08000000)
#define AVB_1722_1_ADP_DEFAULT_AUDIO_FORMAT_88K2						(0x10000000)
#define AVB_1722_1_ADP_DEFAULT_AUDIO_FORMAT_96K							(0x20000000)
#define AVB_1722_1_ADP_DEFAULT_AUDIO_FORMAT_176K4						(0x40000000)
#define AVB_1722_1_ADP_DEFAULT_AUDIO_FORMAT_192K						(0x80000000)


typedef enum {
	ACMP_CMD_CONNECT_TX_COMMAND = 0,
	ACMP_CMD_CONNECT_TX_RESPONSE = 1,
	ACMP_CMD_DISCONNECT_TX_COMMAND = 2,
	ACMP_CMD_DISCONNECT_TX_RESPONSE = 3,
	ACMP_CMD_GET_TX_STATE_COMMAND = 4,
	ACMP_CMD_GET_TX_STATE_RESPONSE = 5,
	ACMP_CMD_CONNECT_RX_COMMAND	= 6,
	ACMP_CMD_CONNECT_RX_RESPONSE = 7,
	ACMP_CMD_DISCONNECT_RX_COMMAND = 8,
	ACMP_CMD_DISCONNECT_RX_RESPONSE = 9,
	ACMP_CMD_GET_RX_STATE_COMMAND = 10,
	ACMP_CMD_GET_RX_STATE_RESPONSE = 11,
	ACMP_CMD_GET_TX_CONNECTION_COMMAND = 12,
	ACMP_CMD_GET_TX_CONNECTION_RESPONSE = 13
} avb_1722_1_acmp_message_type;

typedef enum {
	ACMP_STATUS_SUCCESS = 0,
	ACMP_STATUS_LISTENER_UNKNOWN_ID = 1,
	ACMP_STATUS_TALKER_UNKNOWN_ID = 2,
	ACMP_STATUS_TALKER_DEST_MAC_FAIL = 3,
	ACMP_STATUS_TALKER_NO_STREAM_INDEX = 4,
	ACMP_STATUS_TALKER_NO_BANDWIDTH = 5,
	ACMP_STATUS_TALKER_EXCLUSIVE = 6,
	ACMP_STATUS_LISTENER_TALKER_TIMEOUT = 7,
	ACMP_STATUS_LISTENER_EXCLUSIVE = 8,
	ACMP_STATUS_STATE_UNAVAILABLE = 9,
	ACMP_STATUS_NOT_CONNECTED = 10,
	ACMP_STATUS_NO_SUCH_CONNECTION = 11,
	ACMP_STATUS_COULD_NOT_SEND_MESSAGE = 12,
	ACMP_STATUS_LISTENER_DEFAULT_FORMAT_INVALID = 13,
	ACMP_STATUS_TALKER_DEFAULT_FORMAT_INVALID = 14,
	ACMP_STATUS_DEFAULT_SET_DIFFERENT = 15,
	ACMP_STATUS_NOT_SUPPORTED = 31
} avb_1722_1_acmp_status_type;

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

#define AVB_1722_1_ACMP_FLAGS_CLASS_B				(0x0001)
#define AVB_1722_1_ACMP_FLAGS_FAST_CONNECT			(0x0002)
#define AVB_1722_1_ACMP_FLAGS_SAVED_STATE			(0x0004)

#endif // __AVB_1722_1_PROTOCOL_H__
