#ifndef AVB_1722_1_AECP_AEM_H_
#define AVB_1722_1_AECP_AEM_H_

#include "avb_1722_1_protocol.h"
#include "avb_1722_1_default_conf.h"

/* 7.4.2.1. READ_DESCRIPTOR Command Format */

typedef struct {
    unsigned char configuration[2];
    unsigned char reserved[2];
    unsigned char descriptor_type[2];
    unsigned char descriptor_id[2];
} avb_1722_1_aem_read_descriptor_command_t;

/* 7.4.2.2. READ_DESCRIPTOR Response Format */
typedef struct {
    unsigned char configuration[2];
    unsigned char reserved[2];
    unsigned char descriptor[512];
} avb_1722_1_aem_read_descriptor_response_t;

/* 7.4.1. ACQUIRE_ENTITY Command */
typedef struct {
    unsigned char flags[4];
    unsigned char owner_guid[8];
    unsigned char descriptor_type[2];
    unsigned char descriptor_id[2];
} avb_1722_1_aem_acquire_entity_command_t;

#define AEM_ACQUIRE_ENTITY_PERSISTENT_FLAG(cmd)     ((cmd)->flags[3] & 1)

/* 7.4.2. LOCK_ENTITY Command */
typedef struct {
    unsigned char flags[4];
    unsigned char locked_guid[8];
} avb_1722_1_aem_lock_entity_command_t;

/* 7.4.40.1 GET_AVB_INFO Command */
typedef struct {
    unsigned char descriptor_type[2];
    unsigned char descriptor_id[2];
} avb_1722_1_aem_get_avb_info_command_t;

/* 7.4.40.2 GET_AVB_INFO Response */
typedef struct {
    unsigned char descriptor_type[2];
    unsigned char descriptor_id[2];
    unsigned char as_grandmaster_id[8];
    unsigned char propagation_delay[4];
    unsigned char reserved[2];
    unsigned char msrp_mappings_count[2];
    unsigned char msrp_mappings[4];
} avb_1722_1_aem_get_avb_info_response_t;

/* 7.4.9.1 SET_STREAM_FORMAT Command/response */
typedef struct {
    unsigned char descriptor_type[2];
    unsigned char descriptor_id[2];
    unsigned char stream_format[8];
} avb_1722_1_aem_getset_stream_format_t;

/* 7.4.22. SET_SAMPLING_RATE Command/Response */
typedef struct {
    unsigned char descriptor_type[2];
    unsigned char descriptor_id[2];
    unsigned char sampling_rate[4];
} avb_1722_1_aem_getset_sampling_rate_t;

/* 7.4.23. SET_CLOCK_SOURCE Command/Response */
typedef struct {
    unsigned char descriptor_type[2];
    unsigned char descriptor_id[2];
    unsigned char clock_source_index[2];
    unsigned char reserved[2];
} avb_1722_1_aem_getset_clock_source_t;

/* 7.4.15.1. SET_STREAM_INFO Command/Response */
typedef struct {
    unsigned char descriptor_type[2];
    unsigned char descriptor_id[2];
    unsigned char flags[4];
    unsigned char stream_format[8];
    unsigned char stream_id[8];
    unsigned char msrp_accumulated_latency[4];
    unsigned char stream_dest_mac[6];
    unsigned char msrp_failure_code[1]; 
    unsigned char reserved[1];
    unsigned char msrp_failure_bridge_id[8];
} avb_1722_1_aem_getset_stream_info_t;

/* 7.4.25.1 SET_CONTROL */
typedef struct {
    unsigned char descriptor_type[2];
    unsigned char descriptor_id[2];
} avb_1722_1_aem_getset_control_t;

/* 7.4.35.1 START_STREAMING */
typedef struct {
    unsigned char descriptor_type[2];
    unsigned char descriptor_id[2];
} avb_1722_1_aem_startstop_streaming_t;

/* 7.4.39.1 IDENTIFY_NOTIFICATION */
typedef struct {
    unsigned char descriptor_type[2];
    unsigned char descriptor_index[2];
} avb_1722_1_aem_identify_notification_t;


#endif /* AVB_1722_1_AECP_AEM_H_ */
