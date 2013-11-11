#include <inttypes.h>

typedef struct aem_desc_audio_cluster_t {
    uint8_t descriptor_type[2];
    uint8_t descriptor_index[2];
    uint8_t object_name[64];
    uint8_t localized_description[2];
    uint8_t signal_type[2];
    uint8_t signal_index[2];
    uint8_t signal_output[2];
    uint8_t path_latency[4];
    uint8_t block_latency[4];
    uint8_t channel_count[2];
    uint8_t format[1];
} aem_desc_audio_cluster_t;

typedef struct aem_desc_stream_input_output_t {
    uint8_t descriptor_type[2];
    uint8_t descriptor_index[2];
    uint8_t object_name[64];
    uint8_t localized_description[2];
    uint8_t clock_domain_index[2];
    uint8_t stream_flags[2];
    uint8_t current_format[8];
    uint8_t formats_offset[2];
    uint8_t number_of_formats[2];
    uint8_t backup_talker_entity_id_0[8];
    uint8_t backup_talker_unique_id_0[2];
    uint8_t backup_talker_entity_id_1[8];
    uint8_t backup_talker_unique_id_1[2];
    uint8_t backup_talker_entity_id_2[8];
    uint8_t backup_talker_unique_id_2[2];
    uint8_t backedup_talker_entity_id[8];
    uint8_t backedup_talker_unique_id[2];
    uint8_t avb_interface_index[2];
    uint8_t buffer_length[4];
    #define MAX_NUM_STREAM_FORMATS 8
    uint8_t formats[8*MAX_NUM_STREAM_FORMATS];
    #undef  MAX_NUM_STREAM_FORMATS
} aem_desc_stream_input_output_t;

typedef struct aem_audio_map_format_t {
    uint8_t mapping_stream_index[2];
    uint8_t mapping_stream_channel[2];
    uint8_t mapping_cluster_offset[2];
    uint8_t mapping_cluster_channel[2];
} aem_audio_map_format_t;

typedef struct aem_desc_audio_map_t {
    uint8_t descriptor_type[2];
    uint8_t descriptor_index[2];
    uint8_t mappings_offset[2];
    uint8_t number_of_mappings[2];
    #define MAX_NUM_MAPPINGS 8
    aem_audio_map_format_t mappings[MAX_NUM_MAPPINGS];
    #undef  MAX_NUM_MAPPINGS
} aem_desc_audio_map_t;

typedef struct aem_desc_stream_port_input_output_t {
    uint8_t descriptor_type[2];
    uint8_t descriptor_index[2];
    uint8_t clock_domain_index[2];
    uint8_t port_flags[2];
    uint8_t number_of_controls[2];
    uint8_t base_control[2];
    uint8_t number_of_clusters[2];
    uint8_t base_cluster[2];
    uint8_t number_of_maps[2];
    uint8_t base_map[2];
} aem_desc_stream_port_input_output_t;
