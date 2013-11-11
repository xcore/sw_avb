#ifndef AVB_1722_1_AECPDU_H_
#define AVB_1722_1_AECPDU_H_

#include "avb_1722_1_protocol.h"
#include "avb_1722_1_default_conf.h"
#include "avb_1722_1_aecp_aem.h"

#define AEM_MSG_GET_U_FLAG(pkt)         ((pkt)->uflag_command_type >> 7)
#define AEM_MSG_SET_U_FLAG(pkt, uflag)  ((pkt)->uflag_command_type = ((pkt)->uflag_command_type & 0x7f) | (((uflag) << 7) & 0x80))
#define AEM_MSG_GET_COMMAND_TYPE(pkt)   ((((pkt)->uflag_command_type & 0x7f) << 8)| \
                                        ((pkt)->command_type))

#define AEM_MSG_SET_COMMAND_TYPE(pkt, type) do{ (pkt)->uflag_command_type = ((pkt)->uflag_command_type & 0x80) | (((type) >> 8) & 0x7f); \
                                                (pkt)->command_type = ((type) & 0xFF); } while (0)

#define ADDRESS_MSG_GET_MODE(aa)       (((aa)->mode_length[0] >> 4) & 0xF)
#define ADDRESS_MSG_GET_LENGTH(aa)     ((((aa)->mode_length[0] & 0xf) << 8)| \
                                        ((aa)->mode_length[1]))

#define AVB_1722_1_AECP_CD_LENGTH   40

/**
 * 1722.1 AECP AEM command format
 */
typedef struct {
    unsigned char uflag_command_type;
    unsigned char command_type;
    union {
        avb_1722_1_aem_read_descriptor_command_t read_descriptor_cmd;
        avb_1722_1_aem_read_descriptor_response_t read_descriptor_resp;
        avb_1722_1_aem_acquire_entity_command_t acquire_entity_cmd;
        avb_1722_1_aem_lock_entity_command_t lock_entity_cmd;
        avb_1722_1_aem_get_avb_info_command_t get_avb_info_cmd;
        avb_1722_1_aem_get_avb_info_response_t get_avb_info_resp;
        unsigned char payload[512];
    } command;
} avb_1722_1_aecp_aem_msg_t;

/**
 * 1722.1 AECP Address access format
 */
typedef struct {
    unsigned char tlv_count[2];
    unsigned char mode_length[2];
    unsigned char address[8];
    unsigned char data[514-8-2-2];
} avb_1722_1_aecp_address_access_t;

/**
 * 1722.1 AECP Legacy AV/C command format
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
        avb_1722_1_aecp_aem_msg_t aem;
        avb_1722_1_aecp_address_access_t address;
        avb_1722_1_aecp_avc_t avc;
        avb_1722_1_aecp_vendor_t vendor;
        unsigned char payload[514];
    } data;
} avb_1722_1_aecp_packet_t;

#define AVB_1722_1_AECP_PAYLOAD_OFFSET (sizeof(avb_1722_1_packet_header_t) + 18)
#define AVB_1722_1_AECP_COMMAND_DATA_OFFSET (12)

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
    AECP_AEM_CMD_ACQUIRE_ENTITY = 0,
    AECP_AEM_CMD_LOCK_ENTITY = 1,
    AECP_AEM_CMD_ENTITY_AVAILABLE = 2,
    AECP_AEM_CMD_CONTROLLER_AVAILABLE = 3,
    AECP_AEM_CMD_READ_DESCRIPTOR = 4,
    AECP_AEM_CMD_WRITE_DESCRIPTOR = 5,
    AECP_AEM_CMD_SET_CONFIGURATION = 6,
    AECP_AEM_CMD_GET_CONFIGURATION = 7,
    AECP_AEM_CMD_SET_STREAM_FORMAT = 8,
    AECP_AEM_CMD_GET_STREAM_FORMAT = 9,
    AECP_AEM_CMD_SET_VIDEO_FORMAT = 10,
    AECP_AEM_CMD_GET_VIDEO_FORMAT = 11,
    AECP_AEM_CMD_SET_SENSOR_FORMAT = 12,
    AECP_AEM_CMD_GET_SENSOR_FORMAT = 13,
    AECP_AEM_CMD_SET_STREAM_INFO = 14,
    AECP_AEM_CMD_GET_STREAM_INFO = 15,
    AECP_AEM_CMD_SET_NAME = 16,
    AECP_AEM_CMD_GET_NAME = 17,
    AECP_AEM_CMD_SET_ASSOCIATION_ID = 18,
    AECP_AEM_CMD_GET_ASSOCIATION_ID = 19,
    AECP_AEM_CMD_SET_SAMPLING_RATE = 20,
    AECP_AEM_CMD_GET_SAMPLING_RATE = 21,
    AECP_AEM_CMD_SET_CLOCK_SOURCE = 22,
    AECP_AEM_CMD_GET_CLOCK_SOURCE = 23,
    AECP_AEM_CMD_SET_CONTROL = 24,
    AECP_AEM_CMD_GET_CONTROL = 25,
    AECP_AEM_CMD_INCREMENT_CONTROL = 26,
    AECP_AEM_CMD_DECREMENT_CONTROL = 27,
    AECP_AEM_CMD_SET_SIGNAL_SELECTOR = 28,
    AECP_AEM_CMD_GET_SIGNAL_SELECTOR = 29,
    AECP_AEM_CMD_SET_MIXER = 30,
    AECP_AEM_CMD_GET_MIXER = 31,
    AECP_AEM_CMD_SET_MATRIX = 32,
    AECP_AEM_CMD_GET_MATRIX = 33,
    AECP_AEM_CMD_START_STREAMING = 34,
    AECP_AEM_CMD_STOP_STREAMING = 35,
    AECP_AEM_CMD_REGISTER_UNSOLICITED_NOTIFICATION = 36,
    AECP_AEM_CMD_DEREGISTER_UNSOLICITED_NOTIFICATION = 37,
    AECP_AEM_CMD_IDENTIFY_NOTIFICATION = 38,
    AECP_AEM_CMD_GET_AVB_INFO = 39,
    AECP_AEM_CMD_GET_AS_PATH = 40,
    AECP_AEM_CMD_GET_COUNTERS = 41,
    AECP_AEM_CMD_REBOOT = 42,
    AECP_AEM_CMD_GET_AUDIO_MAP = 43,
    AECP_AEM_CMD_ADD_AUDIO_MAPPINGS = 44,
    AECP_AEM_CMD_REMOVE_AUDIO_MAPPINGS = 45,
    AECP_AEM_CMD_GET_VIDEO_MAP = 46,
    AECP_AEM_CMD_ADD_VIDEO_MAPPINGS = 47,
    AECP_AEM_CMD_REMOVE_VIDEO_MAPPINGS = 48,
    AECP_AEM_CMD_GET_SENSOR_MAP = 49,
    AECP_AEM_CMD_ADD_SENSOR_MAPPINGS = 50,
    AECP_AEM_CMD_REMOVE_SENSOR_MAPPINGS = 51,
    AECP_AEM_CMD_START_OPERATION = 52,
    AECP_AEM_CMD_ABORT_OPERATION = 53,
    AECP_AEM_CMD_OPERATION_STATUS = 54,
    AECP_AEM_CMD_AUTH_ADD_KEY = 55,
    AECP_AEM_CMD_AUTH_DELETE_KEY = 56,
    AECP_AEM_CMD_AUTH_GET_KEY_COUNT = 57,
    AECP_AEM_CMD_AUTH_GET_KEY = 58,
    AECP_AEM_CMD_AUTHENTICATE = 59,
    AECP_AEM_CMD_DEAUTHENTICATE = 60,
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
    AECP_AEM_STATUS_IN_PROGRESS = 9,
    AECP_AEM_STATUS_ENTITY_MISBEHAVING = 10,
    AECP_AEM_STATUS_NOT_SUPPORTED = 11,
    AECP_AEM_STATUS_STREAM_IS_RUNNING = 12,
} avb_1722_1_aecp_aem_status_code;

typedef enum {
    AECP_AA_MODE_READ = 0,
    AECP_AA_MODE_WRITE = 1,
    AECP_AA_MODE_EXECUTE = 2,
} avb_1722_1_aecp_address_access_mode;

typedef enum {
    AECP_AA_STATUS_SUCCESS = 0,
    AECP_AA_STATUS_NOT_IMPLEMENTED = 1,
    AECP_AA_STATUS_ADDRESS_TOO_LOW = 2,
    AECP_AA_STATUS_ADDRESS_TOO_HIGH = 3,
    AECP_AA_STATUS_ADDRESS_INVALID = 4,
    AECP_AA_STATUS_TLV_INVALID = 5,
    AECP_AA_STATUS_DATA_INVALID = 6,
    AECP_AA_STATUS_UNSUPPORTED = 7,
} avb_1722_1_aecp_address_access_status_code;

typedef enum {
    AECP_STATUS_SUCCESS = 0,
    AECP_STATUS_NOT_IMPLEMENTED = 1
} avb_1722_1_aecp_status_type;


#endif /* AVB_1722_1_AECPDU_H_ */
