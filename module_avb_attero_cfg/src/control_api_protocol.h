#ifndef _CONTROL_API_PROTOCOL_H_
#define _CONTROL_API_PROTOCOL_H_
//=============================================================================
//  File Name: control_api_protocol.h
//
//  (c) Copyright [2010] Attero Tech, LLC. All rights reserved
//
//  This source code is Attero Tech, LLC. proprietary and confidential
//      information.
//
//  Description:
//      This function contains the protocol specific defintions for
//      communicating with the Control API via PC application or internally.
//
// Modification History:
//     $Id: control_api_protocol.h,v 1.1 2010/11/12 21:00:41 afoster Exp $
//     $Log: control_api_protocol.h,v $
//     Revision 1.1  2010/11/12 21:00:41  afoster
//     Initial
//
//
//=============================================================================

#define C_API_SRVR_PORT          (ATTERO_CFG_PORT)
#define C_API_MULTICAST_GROUP_IP {224,0,0,100}
#define C_API_GET_ALL_IDS        (-1)
#define C_API_INVALID_ID         (-2)

//=============================================================================
//
// C_API_PACKET_HEADER
//
// The following section defines the structure of the C_API packet header
//
// Byte offset into packet = 0
//
// Packet Header Data:
// Byte 0 - C_API_PROTOCOL_VERSION
// Byte 1 - PACKET_TYPE (upper)
// Byte 2 - PACKET_TYPE (lower)
// Byte 3 - PACKET_ACTION
//
//=============================================================================
#define C_API_PROTOCOL_VERSION     (0)
#define C_API_PROTOCOL_VERSION_LEN (1)

#define C_API_DEVICE_PACKET        (0x0001)
#define C_API_AVB_SOURCE_PACKET    (0x0002)
#define C_API_AVB_SINK_PACKET      (0x0004)
#define C_API_CONTROL_CMD_PACKET   (0x0008)
#define CONTORL_API_PACKET_TYPE_MASK     (0x000F)
#define C_API_PACKET_TYPE_LEN      (2)

#define C_API_PACKET_ACTION_GET    (0x01)
#define C_API_PACKET_ACTION_SET    (0x02)
#define C_API_PACKET_ACTION_CMD    (0x04)
#define C_API_PACKET_ACTION_LEN    (1)

#define C_API_PACKET_HEADER_LEN    (4)
//=============================================================================


//=============================================================================
//
// C_API_SECTION_HEADER
//
// The following section defines the structure of the C_API section header
//
// Byte offset into packet = C_API_PACKET_HEADER_LEN
//
// Section Header Data:
// Byte 0 - SECTION_TAG (upper)
// Byte 1 - SECTION_TAG (lower)
// Byte 2 - SECTION_LEN (upper)
// Byte 3 - SECTION_LEN (lower)
//
//=============================================================================
#define C_API_DEVICE_TAG         (0x0001)
#define C_API_AVB_SOURCE_TAG     (0x0002)
#define C_API_AVB_SINK_TAG       (0x0004)
#define C_API_CONTROL_CMD_TAG    (0x0008)
#define C_API_SECTION_TAG_LEN    (2)

#define C_API_SECTION_LEN        (2)

#define C_API_SECTION_HEADER_LEN (4)
//=============================================================================

//=============================================================================
//
// C_API_DEVICE
//
// The following section defines the structure of the C_API_DEVICE
//
// Section Data:
//
// Byte 0 - device_name[0]
//  ...
// Byte 31 - device_name[C_API_DEVICE_NAME_LEN-1]

// Byte 32 - num_sources(MSB)
// Byte 33 - num_sources(LSB)
//
// Byte 34 - num_sinks(MSB)
// Byte 35 - num_sinks(LSB)
//
//=============================================================================
#define C_API_DEVICE_NAME_LEN (AVB_MAX_NAME_LEN)
#define C_API_NUM_SOURCES_LEN (2)
#define C_API_NUM_SINKS_LEN   (2)

typedef struct
{
    unsigned char device_name[C_API_DEVICE_NAME_LEN];

    unsigned short num_sources;
    unsigned short num_sinks;

}c_api_device_t;
#define C_API_DEVICE_MAX_LEN (36)
//=============================================================================


//=============================================================================
//
// C_API_AVB_SOURCE
//
// The following section defines the structure of the C_API_AVB_SOURCE
// (set) command
//
// Section Data:
// Byte 0  - stream_id[0] (MSB)
// Byte 1  - stream_id[0] (MSB-1)
// Byte 2  - stream_id[0] (LSB+1)
// Byte 3  - stream_id[0] (LSB)
//
// Byte 4  - stream_id[1] (MSB)
// Byte 5  - stream_id[1] (MSB-1)
// Byte 6  - stream_id[1] (LSB+1)
// Byte 7  - stream_id[1] (LSB)
//
// Byte 8  - num_channels (MSB)
// Byte 9  - num_channels (MSB-1)
// Byte 10 - num_channels (LSB+1)
// Byte 11 - num_channels (LSB)
//
// Byte 12 - local_id (MSB)
// Byte 13 - local_id (MSB-1)
// Byte 14 - local_id (LSB+1)
// Byte 15 - local_id (LSB)
//
// Byte 16 - map[0] (MSB)
// Byte 17 - map[0] (MSB-1)
// Byte 18 - map[0] (LSB+1)
// Byte 19 - map[0] (LSB)
//
// ...
//
// Byte 44 - map[C_API_STREAM_MAP_LEN-1] (MSB)
// Byte 45 - map[C_API_STREAM_MAP_LEN-1] (MSB-1)
// Byte 46 - map[C_API_STREAM_MAP_LEN-1] (LSB+1)
// Byte 47 - map[C_API_STREAM_MAP_LEN-1] (LSB)
//
//=============================================================================
#define C_API_STREAM_ID_LEN           (8)
#define C_API_STREAM_NUM_CHANNELS_LEN (4)
#define C_API_STREAM_LOCAL_ID_LEN     (4)
#define C_API_STREAM_MAP_LEN          (8)

typedef struct
{
  signed int    stream_id[2];
  signed int    num_channels;
  signed int    local_id;
  signed int    map[C_API_STREAM_MAP_LEN];
}c_api_set_avb_source_t;
#define C_API_AVB_SOURCE_MAX_LEN (48)

//=============================================================================
// The following section defines the structure of the C_API_AVB_SOURCE
// (get) The command structure is the same for C_API_AVB_SINK (get)
//
// Section Data:
//
// Byte 0 - local_id (MSB)
// Byte 1 - local_id (MSB-1)
// Byte 2 - local_id (LSB+1)
// Byte 3 - local_id (LSB)
//
//=============================================================================
typedef struct
{
  signed int    local_id;
}c_api_get_avb_sink_source_t;

#define C_API_GET_AVB_SS_MAX_LEN (4)
//=============================================================================


//=============================================================================
//
// C_API_AVB_SINK
//
// The following section defines the structure of the C_API_AVB_SINK
// (set) command.
//
// Section Data:
// Byte 0  - stream_id[0] (MSB)
// Byte 1  - stream_id[0] (MSB-1)
// Byte 2  - stream_id[0] (LSB+1)
// Byte 3  - stream_id[0] (LSB)
//
// Byte 4  - stream_id[1] (MSB)
// Byte 5  - stream_id[1] (MSB-1)
// Byte 6  - stream_id[1] (LSB+1)
// Byte 7  - stream_id[1] (LSB)
//
// Byte 8  - num_channels (MSB)
// Byte 9  - num_channels (MSB-1)
// Byte 10 - num_channels (LSB+1)
// Byte 11 - num_channels (LSB)
//
// Byte 12 - local_id (MSB)
// Byte 13 - local_id (MSB-1)
// Byte 14 - local_id (LSB+1)
// Byte 15 - local_id (LSB)
//
// Byte 16 - map[0] (MSB)
// Byte 17 - map[0] (MSB-1)
// Byte 18 - map[0] (LSB+1)
// Byte 19 - map[0] (LSB)
//
// ...
//
// Byte 44 - map[C_API_STREAM_MAP_LEN-1] (MSB)
// Byte 45 - map[C_API_STREAM_MAP_LEN-1] (MSB-1)
// Byte 46 - map[C_API_STREAM_MAP_LEN-1] (LSB+1)
// Byte 47 - map[C_API_STREAM_MAP_LEN-1] (LSB)
//
//=============================================================================
typedef struct
{
  signed int    stream_id[2];
  signed int    num_channels;
  signed int    local_id;
  signed int    map[C_API_STREAM_MAP_LEN];
}c_api_set_avb_sink_t;
#define C_API_AVB_SINK_MAX_LEN (48)
//=============================================================================

//=============================================================================
//
// C_API_CONTROL_CMD_HEADER
//
// The following section defines the structure of the
// C_API_CONTROL_CMD_HEADER
//
//
// Section Data:
//
// Byte 0 - cmd_id (upper)
// Byte 1 - cmd_id (lower)
//
// Byte 2 - cmd_len (upper)
// Byte 3 - cmd_len (lower)
//
//=============================================================================
#define C_API_CMD_ID_LEN  (2)
#define C_API_CMD_LEN_LEN (2)

#define C_API_CMD_REMOTE_BUTTON_ID     (0x0001)
#define C_API_CMD_REMOTE_LED_ID        (0x0002)
#define C_API_CMD_MUTE_ID              (0x0003)
#define C_API_CMD_STREAM_SEL_BUTTON_ID (0x0004)

typedef struct
{
  unsigned short cmd_id;
  unsigned short cmd_len;
}c_api_cmd_header_t;

#define C_API_CMD_HEADER_MAX_LEN (4)
//=============================================================================


//=============================================================================
//
// C_API_CMD_REMOTE_BUTTON
//
// The following section defines the structure of the
// C_API_CMD_REMOTE_BUTTON
//
//
// Section Data:
//
// Byte 0 - button_state
//
//=============================================================================
#define C_API_ARG_BUTTON_STATE_LEN (1)
#define C_API_BUTTON_RELEASED      (0x00)
#define C_API_BUTTON_PRESSED       (0x01)

typedef struct
{
    unsigned char button_state;
}c_api_cmd_remote_button_t;

#define C_API_CMD_REMOTE_BUTTON_MAX_LEN (1)
//=============================================================================


//=============================================================================
//
// C_API_CMD_REMOTE_LED
//
// The following section defines the structure of the C_API_CMD_REMOTE_LED
//
//
// Section Data:
//
// Byte 0 - led_state
//
//=============================================================================
#define C_API_ARG_LED_STATE_LEN (1)
#define C_API_LED_OFF      (0x00)
#define C_API_LED_ON       (0x01)

typedef struct
{
    unsigned char led_state;
}c_api_cmd_remote_led_t;

#define C_API_CMD_REMOTE_LED_MAX_LEN (1)
//=============================================================================


//=============================================================================
//
// C_API_CMD_MUTE
//
// The following section defines the structure of the C_API_CMD_MUTE
//
//
// Section Data:
//
// Byte 0 - mute_state
//
//=============================================================================
#define C_API_ARG_MUTE_STATE_LEN (1)
#define C_API_MUTE_OFF           (0x00)
#define C_API_MUTE_ON            (0x01)

typedef struct
{
    unsigned char mute_state;
}C_API_cmd_mute_t;

#define C_API_CMD_MUTE_MAX_LEN (1)

//=============================================================================


//=============================================================================
//
// C_API_CMD_STREAM_SEL_BUTTON
//
// The following section defines the structure of the
// C_API_CMD_STREAM_SEL_BUTTON
//
// Section Data:
//
// Byte 0 - button_state
//
//=============================================================================
typedef struct
{
    unsigned char button_state;
}c_api_cmd_st_sel_button_t;

#define C_API_CMD_ST_SEL_BUTTON_MAX_LEN (1)
//=============================================================================

typedef struct
{
    unsigned char *raw_data_ptr;
    unsigned char *data_ptr;
    unsigned short raw_data_len;
    unsigned short packet_type;
    unsigned char  packet_action;
    unsigned char  protocol_version;
    unsigned short section_tag;
    unsigned short section_length;
}c_api_packet_args_t;

//typedef void (*packet_handler)(c_api_packet_args_t *p_args_ptr);

#endif
