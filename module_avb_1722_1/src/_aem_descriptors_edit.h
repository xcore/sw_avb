#include "aem_descriptor_types.h"
#include "aem_entity_strings.h"
#include "avb_srp_pdu.h"
#include "avb_1722_def.h"
#include "avb_1722_1_adp_pdu.h"
#include "avb_conf.h"

#define U16(data) (unsigned char)((data) >> 8), (unsigned char)((data) & 0xff)
#define U32(data) (unsigned char)(((data) >> 24) & 0xff), (unsigned char)(((data) >> 16) & 0xff), (unsigned char)(((data) >> 8 ) & 0xff), (unsigned char)(data)
#define U64(data) (unsigned char)(((data) >> 56) & 0xff), (unsigned char)(((data) >> 48) & 0xff), (unsigned char)(((data) >> 40 ) & 0xff), (unsigned char)(((data) >> 32 ) & 0xff), U32(data)

#ifdef AVB_1722_1_AEM_ENABLED

/* Entity Descriptor */
unsigned char desc_entity[] = 
{                                                                                                                                                            
  U16(AEM_ENTITY_TYPE),                       /* 0-1 descriptor_type */                
  U16(0),                                     /* 2-3 descriptor_id */  
  0, 0, 0, 0, 0, 0, 0, 0,                     /* 4-11 entity_guid */ 
  U32(AVB_1722_1_ADP_VENDOR_ID),              /* 12-15 vendor_id */
  U32(AVB_1722_1_ADP_MODEL_ID),               /* 16-19 model_id */
  U32(AVB_1722_1_ADP_ENTITY_CAPABILITIES),    /* 20-23 entity_capabilities */
  U16(AVB_1722_1_ADP_TALKER_STREAM_SOURCES),  /* 24-25 talker_stream_sources */
  U16(AVB_1722_1_ADP_TALKER_CAPABILITIES),    /* 26-27 talker_capabilities */
  U16(AVB_1722_1_ADP_LISTENER_STREAM_SINKS),  /* 28-29 listener_stream_sinks */
  U16(AVB_1722_1_ADP_LISTENER_CAPABILITIES),  /* 30-31 listener_capabilities */
  U32(AVB_1722_1_ADP_CONTROLLER_CAPABILITIES),/* 32-35 controller_capabilities */
  0, 0, 0, 0,                                 /* 36-39 available_index */
  0, 0, 0, 0, 0, 0, 0, 0,                     /* 40-47 as_grandmaster_id */
  0, 0, 0, 0, 0, 0, 0, 0,                     /* 48-55 associated_id */
  U32(AVB_1722_1_ADP_ENTITY_TYPE_OTHER),      /* 56-59 entity_type */
  AVB_1722_1_ENTITY_NAME_STRING,              /* 60-123 entity_name */
  U16(0),                                     /* 124-125 vendor_name_string */
  U16(1),                                     /* 126-127 model_name_string */
  AVB_1722_1_FIRMWARE_VERSION_STRING,         /* 128-191 firmware_version */
  AVB_1722_1_GROUP_NAME_STRING,               /* 192-255 group_name */
  AVB_1722_1_SERIAL_NUMBER_STRING,            /* 256-319 serial_number */
  U16(1),                                     /* 320-321 configurations_count */
  U16(0)                                      /* 322-323 current_configuration */
};   

/* Configuration Descriptor 0 */

unsigned char desc_configuration_0[] =
{
  U16(AEM_CONFIGURATION_TYPE),                /* 0-1 descriptor_type */
  U16(0),                                     /* 2-3 descriptor_id */
  "Configuration 0",                          /* 4-67 configuration_name */
  U16(AEM_NO_STRING),                         /* 68-69 configuration_name_string */
  U16(9),                                     /* 70-71 descriptor_counts_count */
  U16(74),                                    /* 72-73 descriptor_counts_offset */
  /* 74-> descriptor_counts */
  U16(AEM_AUDIO_UNIT_TYPE),
  U16(1),
  U16(AEM_STREAM_INPUT_TYPE),
  U16(1),
  U16(AEM_STREAM_OUTPUT_TYPE),
  U16(1),
  U16(AEM_JACK_INPUT_TYPE),
  U16(1),
  U16(AEM_JACK_OUTPUT_TYPE),
  U16(1),
  U16(AEM_AVB_INTERFACE_TYPE),
  U16(1),
  U16(AEM_CLOCK_SOURCE_TYPE),
  U16(1),
  U16(AEM_CONTROL_TYPE),
  U16(1),
  U16(AEM_LOCALE_TYPE),
  U16(1)
};

/* Audio Unit Descriptor 0 */

unsigned char desc_audio_unit_0[] =
{
  U16(AEM_AUDIO_UNIT_TYPE),                   /* 0-1 descriptor_type */
  U16(0),                                     /* 2-3 descriptor_id */
  U16(1),                                     /* 4-5 number_of_stream_input_ports */
  U16(0),                                     /* 6-7 base_stream_input_port */
  U16(1),                                     /* 8-9 number_of_stream_output_ports */
  U16(0),                                     /* 10-11 base_stream_output_port */
  U16(1),                                     /* 12-13 number_of_external_input_ports */
  U16(0),                                     /* 14-15 base_external_input_port */
  U16(1),                                     /* 16-17 number_of_external_output_ports */
  U16(0),                                     /* 18-19 base_external_output_port */
  U16(0),                                     /* 20-21 number_of_internal_input_ports */
  U16(0),                                     /* 22-23 base_internal_input_port */
  U16(0),                                     /* 24-25 number_of_internal_output_ports */
  U16(0),                                     /* 26-27 base_internal_output_port */
  U16(0),                                     /* 28-29 clock_source_id */
  U16(2),                                     /* 30-31 number_of_controls */
  U16(0),                                     /* 32-33 base_control */
  "Audio Unit 0",                             /* 34-97 unit_name */
  U16(AEM_NO_STRING),                         /* 98-99 unit_name_string */
  U16(0),                                     /* 100-101 number_of_signal_selectors */
  U16(0),                                     /* 102-103 base_signal_selector */
  U16(0),                                     /* 104-105 number_of_mixers */
  U16(0),                                     /* 106-107 base_mixer */
  U16(0),                                     /* 108-109 number_of_matrices */
  U16(0),                                     /* 110-111 base_matrix */
  U32(48000),                                 /* 112-115 current_sample_rate */
  U16(120),                                   /* 116-117 sample_rates_offset */
  U16(3),                                     /* 118-119 sample_rates_count */
  /* 120-> sample_rates */
  U32(44100),
  U32(48000),
  U32(96000)
};

/*******************************/

/* Audio Input Port Descriptors */

unsigned char desc_audio_input_port_0[] =
{
  U16(AEM_AUDIO_PORT_INPUT_TYPE),             /* 0-1 descriptor_type */
  U16(0),                                     /* 2-3 descriptor_id */
  U16(0),                                     /* 4-5 port_flags */
  U16(2),                                     /* 6-7 audio_channels */
  U16(2),                                     /* 8-9 number_of_clusters */
  U16(0),                                     /* 10-11 base_cluster */
  U16(0),                                     /* 12-13 base_audio_map */
  U16(1)                                      /* 14-15 number_of_audio_maps */
};

/* Audio Input Clusters */

unsigned char desc_audio_cluster_0[] =
{
  U16(AEM_AUDIO_CLUSTER_TYPE),                /* 0-1 descriptor_type */
  U16(0),                                     /* 2-3 descriptor_id */ 
  U16(1),                                     /* 4-5 channel_count */
  U32(0),                                     /* 6-9 path_latency */
  0x40,                                       /* 10-11 am824_label */ 
  "Left",                                     /* 12-75 cluster_name */
  U16(AEM_NO_STRING),                         /* 76-77 cluster_name_string */
  U16(0),                                     /* 78-79 signal_type */
  U16(0),                                     /* 80-81 signal_id */
  U32(0)                                      /* 82-85 block_latency */
};

unsigned char desc_audio_cluster_1[] =
{
  U16(AEM_AUDIO_CLUSTER_TYPE),                /* 0-1 descriptor_type */
  U16(1),                                     /* 2-3 descriptor_id */ 
  U16(1),                                     /* 4-5 channel_count */
  U32(0),                                     /* 6-9 path_latency */
  0x40,                                       /* 10-11 am824_label */ 
  "Right",                                    /* 12-75 cluster_name */
  U16(AEM_NO_STRING),                         /* 76-77 cluster_name_string */
  U16(0),                                     /* 78-79 signal_type */
  U16(0),                                     /* 80-81 signal_id */
  U32(0)                                      /* 82-85 block_latency */
};

/* Audio Input Map */

unsigned char desc_audio_input_map_0[] =
{
  U16(AEM_AUDIO_MAP_TYPE),                    /* 0-1 descriptor_type */
  U16(0),                                     /* 2-3 descriptor_id */
  U16(8),                                     /* 4-5 mappings_offset */
  U16(2),                                     /* 6-7 number_of_mappings */
  /* 8-> mappings */
  U16(0),                                     /* mapping_stream_index[0] */
  U16(0),                                     /* mapping_stream_channel[0] */
  U16(0),                                     /* mapping_audio_channel[0] */
  U16(0),                                     /* mapping_stream_index[1] */
  U16(1),                                     /* mapping_stream_channel[1] */
  U16(1)                                      /* mapping_audio_channel[1] */
};

/*****************************/

/* Audio Output Port Descriptors */

unsigned char desc_audio_output_port_0[] =
{
  U16(AEM_AUDIO_PORT_OUTPUT_TYPE),            /* 0-1 descriptor_type */
  U16(0),                                     /* 2-3 descriptor_id */
  U16(0),                                     /* 4-5 port_flags */
  U16(2),                                     /* 6-7 audio_channels */
  U16(2),                                     /* 8-9 number_of_clusters */
  U16(2),                                     /* 10-11 base_cluster */
  U16(1),                                     /* 12-13 base_audio_map */
  U16(1)                                      /* 14-15 number_of_audio_maps */
};

/* Audio Output Clusters */

unsigned char desc_audio_cluster_2[] =
{
  U16(AEM_AUDIO_CLUSTER_TYPE),                /* 0-1 descriptor_type */
  U16(2),                                     /* 2-3 descriptor_id */ 
  U16(1),                                     /* 4-5 channel_count */
  U32(0),                                     /* 6-9 path_latency */
  0x40,                                       /* 10-11 am824_label */ 
  "Left",                                     /* 12-75 cluster_name */
  U16(AEM_NO_STRING),                         /* 76-77 cluster_name_string */
  U16(0),                                     /* 78-79 signal_type */
  U16(0),                                     /* 80-81 signal_id */
  U32(0)                                      /* 82-85 block_latency */
};

unsigned char desc_audio_cluster_3[] =
{
  U16(AEM_AUDIO_CLUSTER_TYPE),                /* 0-1 descriptor_type */
  U16(3),                                     /* 2-3 descriptor_id */ 
  U16(1),                                     /* 4-5 channel_count */
  U32(0),                                     /* 6-9 path_latency */
  0x40,                                       /* 10-11 am824_label */ 
  "Right",                                    /* 12-75 cluster_name */
  U16(AEM_NO_STRING),                         /* 76-77 cluster_name_string */
  U16(0),                                     /* 78-79 signal_type */
  U16(0),                                     /* 80-81 signal_id */
  U32(0)                                      /* 82-85 block_latency */
};

/* Audio Output Map */

unsigned char desc_audio_map_1[] =
{
  U16(AEM_AUDIO_MAP_TYPE),                    /* 0-1 descriptor_type */
  U16(1),                                     /* 2-3 descriptor_id */
  U16(8),                                     /* 4-5 mappings_offset */
  U16(2),                                     /* 6-7 number_of_mappings */
  /* 8-> mappings */
  U16(0),                                     /* mapping_stream_index[0] */
  U16(0),                                     /* mapping_stream_channel[0] */
  U16(0),                                     /* mapping_audio_channel[0] */
  U16(0),                                     /* mapping_stream_index[1] */
  U16(1),                                     /* mapping_stream_channel[1] */
  U16(1)                                      /* mapping_audio_channel[1] */
};

/*******************************/

/* Input External Ports */

unsigned char desc_external_input_port_0[] =
{
  U16(AEM_EXTERNAL_PORT_INPUT_TYPE),          /* 0-1 descriptor_type */
  U16(0),                                     /* 2-3 descriptor_id */ 
  U16(0),                                     /* 4-5 port_flags */
  U16(0),                                     /* 6-7 signal_type */
  U16(0),                                     /* 8-9 signal_id */
  U16(0),                                     /* 10-11 jack_id */
  U32(0)                                      /* 12-15 block_latency */
};

/* Output External Ports */

unsigned char desc_external_output_port_0[] =
{
  U16(AEM_EXTERNAL_PORT_OUTPUT_TYPE),         /* 0-1 descriptor_type */
  U16(0),                                     /* 2-3 descriptor_id */ 
  U16(0),                                     /* 4-5 port_flags */
  U16(0),                                     /* 6-7 signal_type */
  U16(0),                                     /* 8-9 signal_id */
  U16(0),                                     /* 10-11 jack_id */
  U32(0)                                      /* 12-15 block_latency */
};

/* Control Desciptors */

/* Mute */
unsigned char desc_control_mute[] =
{
  U16(AEM_CONTROL_TYPE),                      /* 0-1 descriptor_type */
  U16(0),                                     /* 2-3 descriptor_id */
  U64(AEM_CONTROL_TYPE_MUTE),                 /* 4-11 control_type */
  U16(AEM_EXTERNAL_PORT_OUTPUT_TYPE),         /* 12-13 control_location_type */
  U16(0),                                     /* 14-15 control_location_id */
  U16(AEM_CONTROL_LINEAR_UINT8),              /* 16-17 control_value_type */
  U16(0),                                     /* 18-19 control_domain */
  "Mute",                                     /* 20-83 control_name */
  U16(AEM_NO_STRING),                        /* 84-85 control_name_string */
  U16(102),                                   /* 86-87 values_offset */
  U16(1),                                     /* 88-89 number_of_values */
  U16(0),                                     /* 90-91 signal_type */
  U16(0),                                     /* 92-93 signal_id */
  U32(0),                                     /* 94-97 block_latency */
  U32(0),                                     /* 98-101 control_latency */
  /* 102-> value_details */
  0,                                          /* minimum_value[0] */
  255,                                        /* maximum_value[0] */
  255,                                        /* step[0] */
  0,                                          /* default_value[0] */
  0,                                          /* current_value[0] */
  U16(AEM_CONTROL_UNITS_UNITLESS),            /* unit[0] */
  U16(AEM_NO_STRING)                            /* string[0] */
};

/* Volume */
unsigned char desc_control_volume[] =
{
  U16(AEM_CONTROL_TYPE),                      /* 0-1 descriptor_type */
  U16(1),                                     /* 2-3 descriptor_id */
  U64(AEM_CONTROL_TYPE_VOLUME),               /* 4-11 control_type */
  U16(AEM_EXTERNAL_PORT_OUTPUT_TYPE),         /* 12-13 control_location_type */
  U16(0),                                     /* 14-15 control_location_id */
  U16(AEM_CONTROL_LINEAR_UINT32),             /* 16-17 control_value_type */
  U16(0),                                     /* 18-19 control_domain */
  "Volume",                                   /* 20-83 control_name */
  U16(AEM_NO_STRING),                         /* 84-85 control_name_string */
  U16(102),                                   /* 86-87 values_offset */
  U16(1),                                     /* 88-89 number_of_values */
  U16(0),                                     /* 90-91 signal_type */
  U16(0),                                     /* 92-93 signal_id */
  U32(0),                                     /* 94-97 block_latency */
  U32(0),                                     /* 98-101 control_latency */
  /* 102-> value_details */
  U32(0),                                     /* minimum_value[0] */
  U32(0),                                     /* maximum_value[0] */
  U32(0),                                     /* step[0] */
  U32(0),                                     /* default_value[0] */
  U32(0),                                     /* current_value[0] */
  U16(AEM_CONTROL_UNITS_UNITLESS),            /* unit[0] */
  U16(AEM_NO_STRING)                            /* string[0] */
};

/* Stream Descriptors */

/* Input */
unsigned char desc_stream_input_0[] =
{
  U16(AEM_STREAM_INPUT_TYPE),                 /* 0-1 descriptor_type */
  U16(0),                                     /* 2-3 descriptor_id */
  "Input Stream 0"                            /* 4-67 stream_name */
  U16(AEM_NO_STRING),                         /* 68-69 stream_name_string */
  U16(AEM_STREAM_FLAGS_CLASS_A),              /* 70-71 stream_flags */
  U16(2),                                     /* 72-73 stream_channels */
  U16(0),                                     /* 74-75 clock_source_id */
  U64(0x0050020200000000),                    /* 76-83 current_format */
  U16(130),                                   /* 84-85 formats_offset */
  U16(3),                                     /* 86-87 number_of_formats */
  0, 0, 0, 0, 0, 0, 0, 0,                     /* 88-95 backup_talker_guid[0] */
  U16(0),                                     /* 96-97 backup_talker_unique[0] */
  0, 0, 0, 0, 0, 0, 0, 0,                     /* 98-105 backup_talker_guid[1] */
  U16(0),                                     /* 106-107 backup_talker_unique[1] */
  0, 0, 0, 0, 0, 0, 0, 0,                     /* 108-115 backup_talker_guid[2] */
  U16(0),                                     /* 116-117 backup_talker_unique[2] */
  0, 0, 0, 0, 0, 0, 0, 0,                     /* 118-125 backedup_talker_guid[0] */
  U16(0),                                     /* 126-127 backedup_talker_unique[0] */
  U16(0),                                     /* 128-129 avb_interface_id */
  /* 130-> formats */
  U64(0x0050020200000000),
  U64(0x0050040200000000),
  U64(0x0050060200000000)
};

/* Output */
unsigned char desc_stream_output_0[] =
{
  U16(AEM_STREAM_OUTPUT_TYPE),                /* 0-1 descriptor_type */
  U16(0),                                     /* 2-3 descriptor_id */
  "Output Stream 0"                           /* 4-67 stream_name */
  U16(AEM_NO_STRING),                         /* 68-69 stream_name_string */
  U16(AEM_STREAM_FLAGS_CLASS_A),              /* 70-71 stream_flags */
  U16(2),                                     /* 72-73 stream_channels */
  U16(0),                                     /* 74-75 clock_source_id */
  U64(0x0050020200000000),                    /* 76-83 current_format */
  U16(130),                                   /* 84-85 formats_offset */
  U16(3),                                     /* 86-87 number_of_formats */
  0, 0, 0, 0, 0, 0, 0, 0,                     /* 88-95 backup_talker_guid[0] */
  U16(0),                                     /* 96-97 backup_talker_unique[0] */
  0, 0, 0, 0, 0, 0, 0, 0,                     /* 98-105 backup_talker_guid[1] */
  U16(0),                                     /* 106-107 backup_talker_unique[1] */
  0, 0, 0, 0, 0, 0, 0, 0,                     /* 108-115 backup_talker_guid[2] */
  U16(0),                                     /* 116-117 backup_talker_unique[2] */
  0, 0, 0, 0, 0, 0, 0, 0,                     /* 118-125 backedup_talker_guid[0] */
  U16(0),                                     /* 126-127 backedup_talker_unique[0] */
  U16(0),                                     /* 128-129 avb_interface_id */
  /* 130-> formats */
  U64(0x0050020200000000),
  U64(0x0050040200000000),
  U64(0x0050060200000000)
};

/* Jack Descriptors */

/* Input */
unsigned char desc_jack_input_0[] =
{
  U16(AEM_JACK_INPUT_TYPE),                   /* 0-1 descriptor_type */
  U16(0),                                     /* 2-3 descriptor_id */
  "3.5mm Stereo Jack",                        /* 4-67 jack_name */
  U16(AEM_NO_STRING),                         /* 68-69 jack_name_string */
  U16(0),                                     /* 70-71 jack_flags */
  U16(AEM_JACK_TYPE_UNBALANCED_ANALOG)        /* 72-73 jack_type */
};

/* Output */
unsigned char desc_jack_output_0[] =
{
  U16(AEM_JACK_OUTPUT_TYPE),                  /* 0-1 descriptor_type */
  U16(0),                                     /* 2-3 descriptor_id */
  "3.5mm Stereo Jack",                        /* 4-67 jack_name */
  U16(AEM_NO_STRING),                         /* 68-69 jack_name_string */
  U16(0),                                     /* 70-71 jack_flags */
  U16(AEM_JACK_TYPE_UNBALANCED_ANALOG)        /* 72-73 jack_type */
};

/* AVB Interface Descriptor */
unsigned char desc_avb_interface_0[] =
{
  U16(AEM_AVB_INTERFACE_TYPE),                /* 0-1 descriptor_type */
  U16(0),                                     /* 2-3 descriptor_id */
  0, 0, 0, 0, 0, 0,                           /* 4-9 mac_address */
  0, 0, 0, 0, 0, 0, 0, 0,                     /* 10-17 as_grandmaster_id */
  U16(88),                                    /* 18-19 msrp_mappings_offset */
  U16(1),                                     /* 20-21 msrp_mappings_count */
  "en0",                                      /* 22-85 interface_name */
  U16(AEM_NO_STRING),                         /* 86-87 interface_name_string */
  /* 88-> msrp_mappings */
  AVB_SRP_SRCLASS_DEFAULT,                    /* traffic_class[0] */
  AVB_SRP_TSPEC_PRIORITY_DEFAULT,             /* priority[0] */
  U16(AVB_DEFAULT_VLAN)                       /* vlan_id[0] */
};

/* Clock Source Descriptor */
unsigned char desc_clock_source_0[] =
{
  U16(AEM_CLOCK_SOURCE_TYPE),                 /* 0-1 descriptor_type */
  U16(0),                                     /* 2-3 descriptor_id */
  "Input Stream",                             /* 4-67 clock_source_name */
  U16(AEM_NO_STRING),                         /* 68-69 clock_source_name_string */
  U16(0),                                     /* 70-71 clock_source_flags */
  U16(AEM_CLOCK_SOURCE_INPUT_STREAM),         /* 72-73 clock_source_type */
  0, 0, 0, 0, 0, 0, 0, 0,                     /* 74-81 clock_source_identifier */
  U16(AEM_STREAM_INPUT_TYPE),                 /* 82-83 clock_source_location_type */
  U16(0)                                      /* 84-85 clock_source_location_id */
};

/* Locale Descriptors */
unsigned char desc_locale_0[] =
{
  U16(AEM_LOCALE_TYPE),                       /* 0-1 descriptor_type */
  U16(0),                                     /* 2-3 descriptor_id */
  "en",                                       /* 4-67 locale_identifier */
  U16(1),                                     /* 68-69 number_of_strings */
  U16(0)                                      /* 70-71 base_strings */
};

/* Strings Descriptors */
unsigned char desc_strings_0[] =
{
  U16(AEM_STRINGS_TYPE),                      /* 0-1 descriptor_type */
  U16(0),                                     /* 2-3 descriptor_id */
  AVB_1722_1_VENDOR_NAME_STRING,
  AVB_1722_1_MODEL_NAME_STRING,
  "",
  "",
  "",
  "",
  ""
};

/* List of descriptors */
/* Format is: descriptor_type, # of descriptors of that type, desc size, descriptor... */
/* Should be ordered by descriptor_type num */
unsigned int aem_descriptor_list[] =
{
  AEM_ENTITY_TYPE, 1, sizeof(desc_entity), (unsigned)desc_entity,
  AEM_CONFIGURATION_TYPE, 1, sizeof(desc_configuration_0), (unsigned)desc_configuration_0,
  AEM_AUDIO_UNIT_TYPE, 1, sizeof(desc_audio_unit_0), (unsigned)desc_audio_unit_0,
  AEM_STREAM_INPUT_TYPE, 1, sizeof(desc_stream_input_0), (unsigned)desc_stream_input_0,
  AEM_STREAM_OUTPUT_TYPE, 1, sizeof(desc_stream_output_0), (unsigned)desc_stream_output_0,
  AEM_JACK_INPUT_TYPE, 1, sizeof(desc_jack_input_0), (unsigned)desc_jack_input_0,
  AEM_JACK_OUTPUT_TYPE, 1, sizeof(desc_jack_output_0), (unsigned)desc_jack_output_0,
  AEM_AUDIO_PORT_INPUT_TYPE, 1, sizeof(desc_audio_input_port_0), (unsigned)desc_audio_input_port_0,
  AEM_AUDIO_PORT_OUTPUT_TYPE, 1, sizeof(desc_audio_output_port_0), (unsigned)desc_audio_output_port_0,
  AEM_EXTERNAL_PORT_INPUT_TYPE, 1, sizeof(desc_external_input_port_0), (unsigned)desc_external_input_port_0,
  AEM_EXTERNAL_PORT_OUTPUT_TYPE, 1, sizeof(desc_external_output_port_0), (unsigned)desc_external_output_port_0,
  AEM_AVB_INTERFACE_TYPE, 1, sizeof(desc_avb_interface_0), (unsigned)desc_avb_interface_0,
  AEM_CLOCK_SOURCE_TYPE, 1, sizeof(desc_clock_source_0), (unsigned)desc_clock_source_0,
  AEM_AUDIO_MAP_TYPE, 2, sizeof(desc_clock_source_0), (unsigned)desc_audio_input_map_0, sizeof(desc_audio_map_1), (unsigned)desc_audio_map_1,
  AEM_AUDIO_CLUSTER_TYPE, 4, sizeof(desc_audio_cluster_0), (unsigned)desc_audio_cluster_0, sizeof(desc_audio_cluster_1), (unsigned)desc_audio_cluster_1, sizeof(desc_audio_cluster_2), (unsigned)desc_audio_cluster_2, sizeof(desc_audio_cluster_3), (unsigned)desc_audio_cluster_3,
  AEM_CONTROL_TYPE, 2, sizeof(desc_control_mute), (unsigned)desc_control_mute, sizeof(desc_control_volume), (unsigned)desc_control_volume, 
  AEM_LOCALE_TYPE, 1, sizeof(desc_locale_0), (unsigned)desc_locale_0,
  AEM_STRINGS_TYPE, 1, sizeof(desc_strings_0), (unsigned)desc_strings_0
};


#endif