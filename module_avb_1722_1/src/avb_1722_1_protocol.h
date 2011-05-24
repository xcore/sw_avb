#ifndef __AVB_1722_1_PROTOCOL_H__
#define __AVB_1722_1_PROTOCOL_H__


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
	short entity_guid_lo[2];
	short entity_guid_hi[2];
	short vendor_id[2];
	short model_id[2];
	short entity_capabilities[2];
	short talker_stream_sources;
	short talker_capabilities;
	short listener_stream_sinks;
	short listener_capabilites;
	short controller_capabilities[2];
	short boot_id[2];
	short reserved[6];
} avb_1722_1_sdp_packet_t;

/**
 *  A 1722.1 Simple Connection Management packet
 *
 * \note all elements 16 bit aligned
 */
typedef struct {
	avb_1722_1_packet_header_t header;
	unsigned stream_id[2];
	unsigned controller_guid[2];
	unsigned talker_guid[2];
	unsigned listener_guid[2];
	short talker_unique_id;
	short listener_unique_id;
	char dest_mac[6];
	short connection_count;
	short sequence_id;
	short flags;
} avb_1722_1_scm_packet_t;

/**
 *  A 1722.1 Simple Enumeration and Control packet
 *
 * \note all elements 16 bit aligned
 */
typedef struct {
	avb_1722_1_packet_header_t header;
	unsigned target_guid[2];
	unsigned controller_guid[2];
	short sequence_id;
	char mode_length_upper;
	char length_lower;
	unsigned mode_specific_data[1];
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



#define GET_1722_1_CD_FLAG(pkt) (pkt->cd_subtype >> 7)
#define GET_1722_1_SUBTYPE(pkt) (pkt->cd_subtype & 0x7f)
#define GET_1722_1_SV(pkt) (pkt->sv_avb_version_msg_type >> 7)
#define GET_1722_1_AVB_VERSION(pkt) ((pkt->sv_avb_version_msg_type & 0x70) >> 4)
#define GET_1722_1_MSG_TYPE(pkt) (pkt->sv_avb_version_msg_type & 0x0f)
#define GET_1722_1_VALID_TIME(pkt) (pkt->valid_time_data_length_hi & 0xf8 >> 3)
#define GET_1722_1_DATALENGTH(pkt) \
   (((pkt->valid_time_data_length_hi & 0x7) << 8) + \
          ((pkt->data_length_lo) << 8))

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

#define SET_WORD(member, data) \
	do { \
		member[0] = (data & 0xffff); \
		member[1] = (data >> 16); \
	} while(0);

typedef enum {
	ENTITY_AVAILABLE = 0,
	ENTITY_DEPARTING = 1,
	ENTITY_DISCOVER = 2
} avb_1722_1_sdp_message_type;

#define AVB_1722_1_SDP_ENTITY_CAPIBILITES_17221_IP         		(0x00000001)
#define AVB_1722_1_SDP_ENTITY_CAPIBILITES_ZERO_CONF        		(0x00000002)
#define AVB_1722_1_SDP_ENTITY_CAPIBILITES_BRIDGED_ENTITY   		(0x00000004)
#define AVB_1722_1_SDP_ENTITY_CAPIBILITES_17221_CONTROL    		(0x00000008)
#define AVB_1722_1_SDP_ENTITY_CAPIBILITES_LEGACY_AVB       		(0x00000010)

#define AVB_1722_1_SDP_TALKER_CAPIBILITES_IMPLEMENTED      		(0x0001)
#define AVB_1722_1_SDP_TALKER_CAPIBILITES_AUDIO_SOURCE     		(0x4000)
#define AVB_1722_1_SDP_TALKER_CAPIBILITES_VIDEO_SOURCE     		(0x8000)

#define AVB_1722_1_SDP_LISTENER_CAPIBILITES_IMPLEMENTED    		(0x0001)
#define AVB_1722_1_SDP_LISTENER_CAPIBILITES_AUDIO_SOURCE   		(0x4000)
#define AVB_1722_1_SDP_LISTENER_CAPIBILITES_VIDEO_SOURCE  	 	(0x8000)

#define AVB_1722_1_SDP_CONTROLLER_CAPIBILITES_IMPLEMENTED  		(0x0001)
#define AVB_1722_1_SDP_CONTROLLER_CAPIBILITES_LAYER3_PROXY 		(0x0002)



#define AVB_1722_1_SCM_MESSAGE_TYPE_CONNECT_TX_COMMAND			(0x00)
#define AVB_1722_1_SCM_MESSAGE_TYPE_CONNECT_TX_RESPONSE			(0x01)
#define AVB_1722_1_SCM_MESSAGE_TYPE_DISCONNECT_TX_COMAMND		(0x02)
#define AVB_1722_1_SCM_MESSAGE_TYPE_DISCONNECT_TX_RESPONSE		(0x03)
#define AVB_1722_1_SCM_MESSAGE_TYPE_GET_TX_STATE_COMMAND		(0x04)
#define AVB_1722_1_SCM_MESSAGE_TYPE_GET_TX_STATE_RESPONSE		(0x05)
#define AVB_1722_1_SCM_MESSAGE_TYPE_CONNECT_RX_COMMAND			(0x06)
#define AVB_1722_1_SCM_MESSAGE_TYPE_CONNECT_RX_RESPONSE			(0x07)
#define AVB_1722_1_SCM_MESSAGE_TYPE_DISCONNECT_RX_COMAMND		(0x08)
#define AVB_1722_1_SCM_MESSAGE_TYPE_DISCONNECT_RX_RESPONSE		(0x09)
#define AVB_1722_1_SCM_MESSAGE_TYPE_GET_RX_STATE_COMMAND		(0x0A)
#define AVB_1722_1_SCM_MESSAGE_TYPE_GET_RX_STATE_RESPONSE		(0x0B)
#define AVB_1722_1_SCM_MESSAGE_TYPE_GET_TX_CONNECTION_COMMAND	(0x0C)
#define AVB_1722_1_SCM_MESSAGE_TYPE_GET_TX_CONNECTION_RESPONSE	(0x0D)

#define AVB_1722_1_SCM_STATUS_SUCCESS							(0x00)
#define AVB_1722_1_SCM_STATUS_LISTENER_UNKNOWN_ID				(0x01)
#define AVB_1722_1_SCM_STATUS_TALKER_UNKNOWN_ID					(0x02)
#define AVB_1722_1_SCM_STATUS_TALKER_DEST_MAC_FAIL				(0x03)
#define AVB_1722_1_SCM_STATUS_TALKER_NO_STREAM_INDEX			(0x04)
#define AVB_1722_1_SCM_STATUS_TALKER_NO_BANDWIDTH				(0x05)
#define AVB_1722_1_SCM_STATUS_TALKER_EXCLUSIVE					(0x06)
#define AVB_1722_1_SCM_STATUS_LISTENER_TALKER_TIMEOUT			(0x07)
#define AVB_1722_1_SCM_STATUS_LISTENER_EXCLUSIVE				(0x08)
#define AVB_1722_1_SCM_STATUS_STATE_UNAVAILABLE					(0x09)
#define AVB_1722_1_SCM_STATUS_NOT_CONNECTED						(0x0A)
#define AVB_1722_1_SCM_STATUS_NO_SUCH_CONNECTION				(0x0B)
#define AVB_1722_1_SCM_STATUS_COULD_NOT_SEND_MESSAGE			(0x0C)
#define AVB_1722_1_SCM_STATUS_NOT_SUPPORTED						(0x1F)

#define AVB_1722_1_SCM_FLAGS_CLASS_B							(0x0001)
#define AVB_1722_1_SCM_FLAGS_FAST_CONNECT						(0x0002)
#define AVB_1722_1_SCM_FLAGS_SAVED_STATE						(0x0004)

#endif // __AVB_1722_1_PROTOCOL_H__
