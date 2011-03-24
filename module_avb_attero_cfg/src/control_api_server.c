//=============================================================================
//  File Name: control_api_server.c
//
//  (c) Copyright [2010] Attero Tech, LLC. All rights reserved
//
//  This source code is Attero Tech, LLC. proprietary and confidential
//      information.
//
//  Description:
//      Add source file description here.
//      ARF TODO add better description for this file.
//
// Modification History:
//     $Id: control_api_server.c,v 1.1 2010/11/12 21:00:41 afoster Exp $
//     $Log: control_api_server.c,v $
//     Revision 1.1  2010/11/12 21:00:41  afoster
//     Initial
//
//
//=============================================================================
#include <xs1.h>
#include <stdio.h>
#include <platform.h>
#include <xccompat.h>
#include <string.h>
#include <print.h>
#include "xtcp_client.h"
#include "avb.h"
#include "avb_conf.h"
#include "avb_device_defines.h"
#include "avb_control_types.h"
#include "avb_control.h"
#include "control_api_protocol.h"
#include "control_api_server.h"
#include "c_api_bswap.h"
#include "sw_comps_common.h"
#include "simple_printf.h"

//-----------------------------------------------------------------------------
// Global objects
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Externs
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Local definitions
//-----------------------------------------------------------------------------
#undef countof
// Return the number of elements in an array
#define countof(x) (sizeof(x)/sizeof(x[0]))

#define conv_stream_id_b_to_i(byte_stream, int_stream) \
    int_stream[0] =                                    \
      (byte_stream[0] << 24) |                         \
      (byte_stream[1] << 16) |                         \
      (byte_stream[2] <<  8) |                         \
      (byte_stream[3] <<  0);                          \
                                                       \
    int_stream[1] =                                    \
      (byte_stream[4] << 24) |                         \
      (byte_stream[5] << 16) |                         \
      (byte_stream[6] << 8)  |                         \
    (byte_stream[7] <<0);


#define conv_stream_id_i_to_b(int_stream, byte_stream) \
    byte_stream[0] = (unsigned char)((int_stream[0]>>24)&0xff);   \
    byte_stream[1] = (unsigned char)((int_stream[0]>>16)&0xff);   \
    byte_stream[2] = (unsigned char)((int_stream[0]>>8 )&0xff);   \
    byte_stream[3] = (unsigned char)((int_stream[0]    )&0xff);   \
                                                       \
    byte_stream[4] = (unsigned char)((int_stream[1]>>24)&0xff);   \
    byte_stream[5] = (unsigned char)((int_stream[1]>>16)&0xff);   \
    byte_stream[6] = (unsigned char)((int_stream[1]>>8 )&0xff);   \
    byte_stream[7] = (unsigned char)((int_stream[1]    )&0xff);

#define C_API_TX_BUF_SIZE  (100)
#define C_API_SRC_TYPE     (0x00)
#define C_API_SINK_TYPE    (0x01)

typedef struct
{
  unsigned char  pending;
  unsigned short length;
  unsigned char  buf[C_API_TX_BUF_SIZE];
}control_tx_buf_t;

//-----------------------------------------------------------------------------
// Local objects
//-----------------------------------------------------------------------------

static void c_api_device_tag(
    c_api_packet_args_t *p_args_ptr
);
static unsigned char build_send_device_packet(
    chanend tcp_svr
);

static void c_api_avb_source_tag(
    c_api_packet_args_t *p_args_ptr
);
static unsigned char build_send_avb_source_sink_packet(
    chanend tcp_svr,
    unsigned char type,
    signed int local_id
);
static void c_api_avb_sink_tag(
    c_api_packet_args_t *p_args_ptr
);

static void c_api_control_cmd_tag(
    c_api_packet_args_t *p_args_ptr
);

static void c_api_process_packet(
    unsigned char  *raw_data_ptr, //char pointer to the raw packet data
    unsigned short  raw_data_len  //length of raw data to process.
);
static void c_api_server_recv(
     chanend tcp_svr,         // chan for the xtcp server
     xtcp_connection_t *conn // reference to the connection
);

static control_tx_buf_t m_cont_api_tx_buf;

static unsigned short m_cmds_to_send;
static signed int m_avb_source_req_id;
static signed int m_avb_sink_req_id;

void simple_buf_dump(unsigned char *data, unsigned short data_len);

void simple_buf_dump(unsigned char *data, unsigned short data_len)
{
    unsigned short local_len = 0;
    while (data_len--)
    {
      simple_printf("%x ", *data);
      data++;
      local_len++;
      if ( local_len %16 ==0 )
      {
       local_len =0;
       simple_printf("\n");
      }
    }
    simple_printf("\n" );
}

//-----------------------------------------------------------------------------
// Function: build_packet_header
//
// Description:
//  This function is used to build the packet header.
//
// Returns:
//    unsigned char - length of packet header.
//
//-----------------------------------------------------------------------------
unsigned char build_packet_header(
    unsigned char *d_ptr,         //data storage pointer
    unsigned short packet_type,   //type of packet to build.
    unsigned char  packet_action  //type of packet action
)
{
    //store the control api protocol version
    *d_ptr = (unsigned char)C_API_PROTOCOL_VERSION;
    d_ptr++;

    //store the packet type
    d_ptr = c_api_put_u_short_be(d_ptr, packet_type);

    //store the packet action type
    *d_ptr = packet_action;

    return (C_API_PACKET_HEADER_LEN);
}// end <build_packet_header>

//-----------------------------------------------------------------------------
// Function: build_section_header
//
// Description:
//  This function is used to build the section header.
//
// Returns:
//    unsigned char - length of section header.
//
//-----------------------------------------------------------------------------
unsigned char build_section_header(
    unsigned char *d_ptr,         //data storage pointer
    unsigned short section_type,  //type of section to build.
    unsigned short section_len    //type of packet action
)
{
    //store the section type
    d_ptr = c_api_put_u_short_be(d_ptr, section_type);

    //store the section len
    d_ptr = c_api_put_u_short_be(d_ptr, section_len);

    return (C_API_SECTION_HEADER_LEN);
}// end <build_section_header>

//-----------------------------------------------------------------------------
// Function: c_api_process_packet
//
// Description:
//  This function is used to process incoming control api packets.
//
// Returns:
//    void
//
//-----------------------------------------------------------------------------
void c_api_process_packet(
    unsigned char  *raw_data_ptr, //char pointer to the raw packet data
    unsigned short  raw_data_len  //length of raw data to process.
)
{
    c_api_packet_args_t p_args;

    if ( NULL != raw_data_ptr &&
         C_API_PACKET_HEADER_LEN < raw_data_len  )
    {
        simple_printf("Data_len = %x\n", raw_data_len);
        simple_buf_dump(raw_data_ptr, raw_data_len);
        p_args.raw_data_ptr = raw_data_ptr;
        p_args.raw_data_len = raw_data_len;
        p_args.data_ptr     = raw_data_ptr;

        //store the protocol version
        p_args.protocol_version = *p_args.data_ptr++;

        //read the cmd_id
        p_args.data_ptr = c_api_get_u_short_be(p_args.data_ptr,
                                              &p_args.packet_type);

        //store the packet action
        p_args.packet_action = *p_args.data_ptr++;

        //Eventually we will want support for multiple packet types
        //in a single packet. For now, just use the section tag to
        //determine what commands are in here.

        //Mask off any unsupported commands;
        p_args.packet_type &= CONTORL_API_PACKET_TYPE_MASK;

        //read the section_tag
        p_args.data_ptr = c_api_get_u_short_be(p_args.data_ptr,
                                               &p_args.section_tag);

        //read the section_length
        p_args.data_ptr = c_api_get_u_short_be(p_args.data_ptr,
                                               &p_args.section_length);

        simple_printf("Section: %x \n Section L: %x\n", p_args.section_tag, p_args.section_length);
        switch ( p_args.section_tag )
        {
            case C_API_DEVICE_TAG:
            {
                c_api_device_tag(&p_args);
                break;
            }// end case C_API_DEVICE_TAG:
            case C_API_AVB_SOURCE_TAG:
            {
                c_api_avb_source_tag(&p_args);
                break;
            }// end case C_API_AVB_SOURCE_TAG:
            case C_API_AVB_SINK_TAG:
            {
                c_api_avb_sink_tag(&p_args);
                break;
            }// end case C_API_AVB_SINK_TAG:
            case C_API_CONTROL_CMD_TAG:
            {
                c_api_control_cmd_tag(&p_args);
                break;
            }// end case C_API_CONTROL_CMD_TAG:

            default:
                break;
        }// end switch ( p_args.section_tag )
    }// end if ( NULL != raw_data_ptr && ... )
}// end <c_api_process_packet>

//-----------------------------------------------------------------------------
// Function: c_api_server_init
//
// Description:
//  This function is used to begin listening to the XTCP connection on the
//  desired port for communicating with the PC control client over UDP.
//
// Returns:
//    void
//
//-----------------------------------------------------------------------------
void c_api_server_init(
     chanend tcp_svr // chan for the xtcp server
 )
 {
  xtcp_listen(tcp_svr, C_API_SRVR_PORT, XTCP_PROTOCOL_UDP);
 }// end <c_api_server_init>

//-----------------------------------------------------------------------------
// Function: c_api_server_recv
//
// Description:
//  This function receiving data from the control the current status of the
//  control api parameters.
//
// Returns:
//    void
//
//-----------------------------------------------------------------------------
void c_api_server_recv(
     chanend tcp_svr,         // chan for the xtcp server
     xtcp_connection_t *conn // reference to the connection
)
{
  int len;
  unsigned char data[100];
  simple_printf("Recv \n");
  len = xtcp_recv(tcp_svr, (char *) data);

  c_api_process_packet(data, len);


  if ( 0x0000 != m_cmds_to_send &&
       FALSE  == m_cont_api_tx_buf.pending )
  {
      //There is a command to be sent and none already pending.
      xtcp_init_send(tcp_svr, conn);
  }// end if ( 0x0000 != m_cmds_to_send && ... )
}// end <c_api_server_recv>


//-----------------------------------------------------------------------------
// Function: c_api_server_send
//
// Description:
//  This function builds/sends the current status of the control api parameters.
//
// Returns:
//    void
//
//-----------------------------------------------------------------------------
void c_api_server_send(
     chanend tcp_svr,         // chan for the xtcp server
     xtcp_connection_t *conn, // reference to the connection
     unsigned int t           // event timestamp in units of core frequency
)
{
    unsigned char sent;
    simple_printf("send \n");
    if ( C_API_DEVICE_PACKET & m_cmds_to_send )
    {
        sent = build_send_device_packet(tcp_svr);
        if (sent)
        {
            m_cmds_to_send &= ~C_API_DEVICE_PACKET;
            return;
        }// end if if (sent)
    }// end if ( C_API_DEVICE_PACKET & m_cmds_to_send )

    if ( C_API_AVB_SOURCE_PACKET & m_cmds_to_send )
    {
        sent = build_send_avb_source_sink_packet(tcp_svr,
                                                 0,
                                                 m_avb_source_req_id);
        if (sent)
        {
            if ( m_avb_source_req_id >= (AVB_NUM_SOURCES-1) )
            {
                m_cmds_to_send &= ~C_API_AVB_SOURCE_PACKET;
                m_avb_source_req_id =- C_API_INVALID_ID;
            }
            else
            {
                m_avb_source_req_id++;
            }
            return;
        }// end if if (sent)

    }// end if ( C_API_AVB_SOURCE_PACKET & m_cmds_to_send )

    if ( C_API_AVB_SINK_PACKET & m_cmds_to_send )
    {
        sent = build_send_avb_source_sink_packet(tcp_svr,
                                                 1,
                                                 m_avb_sink_req_id);
        if (sent)
        {
            if ( m_avb_sink_req_id >= (AVB_NUM_SINKS-1) )
            {
                m_cmds_to_send &= ~C_API_AVB_SINK_PACKET;
                m_avb_sink_req_id =- C_API_INVALID_ID;
            }
            else
            {
                m_avb_sink_req_id ++;
            }
            return;
        }// end if if (sent)
    }// end if ( C_API_AVB_SINK_PACKET & m_cmds_to_send )

    if ( C_API_CONTROL_CMD_PACKET & m_cmds_to_send )
    {

    }// end if ( C_API_CONTROL_CMD_PACKET & m_cmds_to_send )

    // nothing to send
    xtcp_send(tcp_svr, NULL, 0);
    simple_printf("send done \n");
    return;
}// end <c_api_server_send>

//-----------------------------------------------------------------------------
// Function: c_api_server_periodic
//
// Description:
//  This function ARF TODO fill in description
//
// Returns:
//    void
//
//-----------------------------------------------------------------------------
void c_api_server_periodic(
     chanend tcp_svr,         // chan for the xtcp server
     xtcp_connection_t *conn, // reference to the connection
     unsigned int t           // event timestamp in units of core frequency
)
{
  //ARF TODO fill this stub with the appropriate periodic function.


}// end <c_api_server_periodic>

//-----------------------------------------------------------------------------
// Function: c_api_handle_udp_event
//
// Description:
//  This function handles UDP events for the control api server.
//
// Returns:
//    void
//
//-----------------------------------------------------------------------------
void c_api_handle_udp_event(
     chanend tcp_svr,         // chan for the xtcp server
     xtcp_connection_t *conn, // reference to the connection
     unsigned int t           // event timestamp in units of core frequency
)
{
  switch (conn->event){
    case XTCP_IFUP:
      {
      }
      return;
    case XTCP_IFDOWN:
    {
       return;
    }
    case XTCP_ALREADY_HANDLED:
      return;
    default:
      break;
  }// end switch (conn->event)

  if (conn->local_port == C_API_SRVR_PORT ){
    switch (conn->event){
      case XTCP_NEW_CONNECTION:
        {
            xtcp_set_poll_interval(tcp_svr, conn, 5000);
        }
        break;
      case XTCP_POLL:
        {
          c_api_server_periodic(tcp_svr, conn, t);
        }
        break;
      case XTCP_RECV_DATA:
        {
            simple_printf("RX: %d.%d.%d.%d \n", conn->remote_addr[0],
                                                conn->remote_addr[1],
                                                conn->remote_addr[2],
                                                conn->remote_addr[3]);
            xtcp_ipaddr_t remote_addr = {conn->remote_addr[0],
                                         conn->remote_addr[1],
                                         conn->remote_addr[2],
                                         conn->remote_addr[3]};

            c_api_server_recv(tcp_svr, conn);
            xtcp_bind_remote(tcp_svr, conn, remote_addr, C_API_SRVR_PORT);

          if ( 0x0000 != m_cmds_to_send &&
               FALSE  == m_cont_api_tx_buf.pending )
          {
              //There is a command to be sent and none already pending.
              xtcp_init_send(tcp_svr, conn);
          }// end if ( 0x0000 != m_cmds_to_send && ... )
        }
        break;
      case XTCP_REQUEST_DATA:
      case XTCP_SENT_DATA:
      case XTCP_RESEND_DATA:
        {
          c_api_server_send(tcp_svr, conn, t);
        }
        break;
      case XTCP_CLOSED:
      {
         c_api_server_init(tcp_svr);
         //simple_printf("init\n");
      }
        break;
      default:
        break;
    }// end switch (conn->event)
    conn->event = XTCP_ALREADY_HANDLED;
  }// end if (conn->local_port == C_API_SRVR_PORT )
  return;
}// end <c_api_handle_udp_event>

//-----------------------------------------------------------------------------
// Function: build_send_device_packet
//
// Description:
//  This function builds the device response and queues the data to be sent.
//
// Returns:
//    void
//
//-----------------------------------------------------------------------------
static unsigned char build_send_device_packet(
    chanend tcp_svr // chan for the xtcp server
)
{
    unsigned char length;
    unsigned char message_built = FALSE;
    unsigned char *d_ptr;

    unsigned int temp_data;
    simple_printf("Send Dev \n");
    if ( FALSE == m_cont_api_tx_buf.pending )
    {
        m_cont_api_tx_buf.pending = TRUE;

        //Build the packet_header
        d_ptr = m_cont_api_tx_buf.buf;
        length =  build_packet_header(d_ptr,
                                      C_API_DEVICE_PACKET,
                                      C_API_PACKET_ACTION_SET);
        d_ptr = d_ptr+length;

        //Build the section header.
        length = build_section_header(d_ptr,
                                      C_API_DEVICE_TAG,
                                      C_API_DEVICE_MAX_LEN);
        d_ptr = d_ptr+length;

        //Put the device name.
        memset(d_ptr, 0x00, C_API_DEVICE_NAME_LEN);
        get_device_name((char *) d_ptr);

        d_ptr = d_ptr+C_API_DEVICE_NAME_LEN;

        //Put the num sources
        get_avb_sources((int *) &temp_data);
        d_ptr = c_api_put_u_short_be(d_ptr, temp_data);

        //put the num_sinks
        get_avb_sinks((int *) &temp_data);
        d_ptr = c_api_put_u_short_be(d_ptr, temp_data);

        m_cont_api_tx_buf.length = d_ptr - m_cont_api_tx_buf.buf;
        message_built = TRUE;
        xtcp_send(tcp_svr,
                  (char *) m_cont_api_tx_buf.buf,
                  m_cont_api_tx_buf.length);
       m_cont_api_tx_buf.pending = FALSE;
    }//end if ( FALSE == m_cont_api_tx_buf.pending )

    return(message_built);
}// end <build_send_device_packet>

//-----------------------------------------------------------------------------
// Function: build_send_avb_source_sink_packet
//
// Description:
//  This function builds the avb source response and queues the data to be sent.
//
// Returns:
//    unsigned char - TRUE  - Message Built
//                  - FALSE - Buffer Busy
//
//-----------------------------------------------------------------------------
static unsigned char build_send_avb_source_sink_packet(
    chanend tcp_svr,    // chan for the xtcp server
    unsigned char type, //source/sink
    signed int local_id //id to send
)
{
    unsigned int stream_id[2];
    int num_channels;
    int map[C_API_STREAM_MAP_LEN];
    int map_len;

    unsigned char length = 0;
    unsigned char *d_ptr;
    unsigned char i;
    unsigned char message_built = FALSE;

    unsigned short packet_type;
    unsigned short section_tag;

    if ( FALSE == m_cont_api_tx_buf.pending )
    {
        m_cont_api_tx_buf.pending = TRUE;
        memset(map, 0xFF, sizeof(map));

        if ( C_API_SRC_TYPE == type )
        {
            packet_type = C_API_AVB_SOURCE_PACKET;
            section_tag = C_API_AVB_SOURCE_TAG;
            get_avb_source_id(local_id, stream_id);
            getset_avb_source_channels(0, local_id, &num_channels);
            getset_avb_source_map(0, local_id, map, &map_len);
        }
        else
        {
            packet_type = C_API_AVB_SINK_PACKET;
            section_tag = C_API_AVB_SINK_TAG;
            get_avb_sink_id(local_id, stream_id);
            getset_avb_sink_channels(0, local_id, &num_channels);
            getset_avb_sink_map(0, local_id, map, &map_len);
        }// end if ( C_API_SRC_TYPE == type )

        //Build the packet_header
        d_ptr = m_cont_api_tx_buf.buf;
        length =  build_packet_header(d_ptr,
                                      packet_type,
                                      C_API_PACKET_ACTION_SET);
        d_ptr = d_ptr+length;

        //Build the section header.
        length = build_section_header(d_ptr,
                                      section_tag,
                                      C_API_AVB_SOURCE_MAX_LEN);
        d_ptr = d_ptr+length;

        //put the source_id
        d_ptr = c_api_put_u_long_be(d_ptr, stream_id[0]);
        d_ptr = c_api_put_u_long_be(d_ptr, stream_id[1]);

        //put the num_channels
        d_ptr = c_api_put_u_long_be(d_ptr, num_channels);

        //put the local_id
        d_ptr = c_api_put_u_long_be(d_ptr, local_id);

        //put the maps
        for (i=0; i<C_API_STREAM_MAP_LEN; i++)
        {
            d_ptr = c_api_put_u_long_be(d_ptr, map[i]);
        }// end for (i=0; i<map_len; i++)

        m_cont_api_tx_buf.length = d_ptr - m_cont_api_tx_buf.buf;
        message_built = TRUE;
        xtcp_send(tcp_svr,
                  (char *) m_cont_api_tx_buf.buf,
                  m_cont_api_tx_buf.length);
       m_cont_api_tx_buf.pending = FALSE;
    }//end if ( FALSE == m_cont_api_tx_buf.pending )

    return(message_built);
}// end <build_send_avb_source_sink_packet>

//-----------------------------------------------------------------------------
// Function: handle_remote_led_cmd
//
// Description:
//  This function handles the remote_led cmd.
//
// Returns:
//    void
//
//-----------------------------------------------------------------------------
static void handle_remote_led_cmd(
    unsigned char *d_ptr, //pointer to the command data
    unsigned int  cmd_len //length of command data
)
{

}// end <handle_remote_led_cmd>

//-----------------------------------------------------------------------------
// Function: handle_mute_cmd
//
// Description:
//  This function handles the mute cmd.
//
// Returns:
//    void
//
//-----------------------------------------------------------------------------
static void handle_mute_cmd(
    unsigned char *d_ptr, //pointer to the command data
    unsigned int  cmd_len //length of command data
)
{

}// end <handle_mute_cmd>

//-----------------------------------------------------------------------------
// Function: c_api_device_tag
//
// Description:
//  The function is used to handle received device packets
//
// Returns:
//    void
//
//-----------------------------------------------------------------------------
static void c_api_device_tag(
    c_api_packet_args_t *p_args_ptr //ptr to packet arguments
)
{
    if ( C_API_PACKET_ACTION_SET == p_args_ptr->packet_action )
    {
        //This action currently not supported
        //TODO: Add support once the stack supports it
    }
    else
    {
        //Default or Get action, build and send the device info.
        m_cmds_to_send |= C_API_DEVICE_PACKET;
    }// end if ( C_API_PACKET_ACTION_SET == p_args_ptr->packet_action )
}// end <c_api_device_tag>

//-----------------------------------------------------------------------------
// Function: c_api_avb_source_tag
//
// Description:
//  The function is used to handle received avb_source packets
//
// Returns:
//    void
//
//-----------------------------------------------------------------------------
static void c_api_avb_source_tag(
    c_api_packet_args_t *p_args_ptr //ptr to packet arguments
)
{
    c_api_set_avb_source_t source;
    unsigned char *d_ptr = p_args_ptr->data_ptr;

    if ( C_API_PACKET_ACTION_SET == p_args_ptr->packet_action &&
         C_API_AVB_SOURCE_MAX_LEN == p_args_ptr->section_length )
    {
         d_ptr = c_api_get_i_long_be(d_ptr, &source.stream_id[0]);
         d_ptr = c_api_get_i_long_be(d_ptr, &source.stream_id[1]);

         d_ptr = c_api_get_i_long_be(d_ptr, &source.num_channels);

         d_ptr = c_api_get_i_long_be(d_ptr, &source.local_id);

         d_ptr = c_api_get_i_long_be(d_ptr, &source.map[0]);
         d_ptr = c_api_get_i_long_be(d_ptr, &source.map[1]);
         d_ptr = c_api_get_i_long_be(d_ptr, &source.map[2]);
         d_ptr = c_api_get_i_long_be(d_ptr, &source.map[3]);
         d_ptr = c_api_get_i_long_be(d_ptr, &source.map[4]);
         d_ptr = c_api_get_i_long_be(d_ptr, &source.map[5]);
         d_ptr = c_api_get_i_long_be(d_ptr, &source.map[6]);
         d_ptr = c_api_get_i_long_be(d_ptr, &source.map[7]);

        simple_printf("Stream[0]: %x\nStream[1]: %x\nLocal ID: %x\nNum Channels: %x\nMap[0]: %x\nMap[1]: %x\nMap[2]: %x\nMap[3]: %x\nMap[4]: %x\nMap[5]: %x\nMap[6]: %x\nMap[7]: %x\n",
                       source.stream_id[0],
                       source.stream_id[1],
                       source.local_id,
                       source.num_channels,
                       source.map[0],
                       source.map[1],
                       source.map[2],
                       source.map[3],
                       source.map[4],
                       source.map[5],
                       source.map[6],
                       source.map[7]);

        //Lets set the stream state first to allow a single command
        //to sequence properly.

        set_avb_source_state(source.local_id, AVB_SOURCE_STATE_DISABLED);

        getset_avb_source_channels(1, source.local_id,
                                     &source.num_channels);

        getset_avb_source_map(1, source.local_id,
                                 source.map,
                                 &source.num_channels);

        set_avb_source_state(source.local_id, AVB_SOURCE_STATE_POTENTIAL);

    }
    else if ( C_API_PACKET_ACTION_GET == p_args_ptr->packet_action )
    {
        //Flag the avb_source info to be built and sent
        m_cmds_to_send |= C_API_AVB_SOURCE_PACKET;
        m_avb_source_req_id = 0;
    }// end if ( C_API_PACKET_ACTION_SET == p_args_ptr->packet_action )
}// end <c_api_avb_source_tag>

//-----------------------------------------------------------------------------
// Function: c_api_avb_sink_tag
//
// Description:
//  The function is used to handle received avb_sink packets
//
// Returns:
//    void
//
//-----------------------------------------------------------------------------
static void c_api_avb_sink_tag(
    c_api_packet_args_t *p_args_ptr //ptr to packet arguments
)
{
    c_api_set_avb_sink_t sink;
    unsigned char *d_ptr = p_args_ptr->data_ptr;

    if ( C_API_PACKET_ACTION_SET == p_args_ptr->packet_action &&
         C_API_AVB_SINK_MAX_LEN  == p_args_ptr->section_length )
    {

        //Lets set the stream state first to allow a single command
        //to sequence properly.
         d_ptr = c_api_get_i_long_be(d_ptr, &sink.stream_id[0]);
         d_ptr = c_api_get_i_long_be(d_ptr, &sink.stream_id[1]);

         d_ptr = c_api_get_i_long_be(d_ptr, &sink.num_channels);

         d_ptr = c_api_get_i_long_be(d_ptr, &sink.local_id);

         d_ptr = c_api_get_i_long_be(d_ptr, &sink.map[0]);
         d_ptr = c_api_get_i_long_be(d_ptr, &sink.map[1]);
         d_ptr = c_api_get_i_long_be(d_ptr, &sink.map[2]);
         d_ptr = c_api_get_i_long_be(d_ptr, &sink.map[3]);
         d_ptr = c_api_get_i_long_be(d_ptr, &sink.map[4]);
         d_ptr = c_api_get_i_long_be(d_ptr, &sink.map[5]);
         d_ptr = c_api_get_i_long_be(d_ptr, &sink.map[6]);
         d_ptr = c_api_get_i_long_be(d_ptr, &sink.map[7]);

        simple_printf("Stream[0]: %x\nStream[1]: %x\nLocal ID: %x\nNum Channels: %x\nMap[0]: %x\nMap[1]: %x\nMap[2]: %x\nMap[3]: %x\nMap[4]: %x\nMap[5]: %x\nMap[6]: %x\nMap[7]: %x\n",
                       sink.stream_id[0],
                       sink.stream_id[1],
                       sink.local_id,
                       sink.num_channels,
                       sink.map[0],
                       sink.map[1],
                       sink.map[2],
                       sink.map[3],
                       sink.map[4],
                       sink.map[5],
                       sink.map[6],
                       sink.map[7]);

        set_avb_sink_state( sink.local_id, AVB_SINK_STATE_DISABLED );

        getset_avb_sink_channels(1, sink.local_id,
                                 (int *) &sink.num_channels);

        getset_avb_sink_id(1, sink.local_id, (unsigned int *) sink.stream_id);
        
        getset_avb_sink_map(1, sink.local_id, sink.map,
                              &sink.num_channels);

        set_avb_sink_state( sink.local_id, AVB_SINK_STATE_POTENTIAL );
    }
    else if ( C_API_PACKET_ACTION_GET == p_args_ptr->packet_action )
    {
        //Flag the avb_sink info to be built and sent
        m_cmds_to_send |= C_API_AVB_SINK_PACKET;
        m_avb_sink_req_id = 0;
    }// end if ( C_API_PACKET_ACTION_SET == p_args_ptr->packet_action )
}// end <c_api_avb_sink_tag>

//-----------------------------------------------------------------------------
// Function: c_api_control_cmd_tag
//
// Description:
//  The function is used to handle received control_cmd packets
//
// Returns:
//    void
//
//-----------------------------------------------------------------------------
static void c_api_control_cmd_tag(
    c_api_packet_args_t *p_args_ptr //ptr to packet arguments
)
{
    unsigned char *d_ptr = p_args_ptr->data_ptr;
    unsigned char *end_ptr = p_args_ptr->data_ptr + p_args_ptr->section_length;

    unsigned short cmd_id;
    unsigned short cmd_len;

    if ( C_API_PACKET_ACTION_CMD == p_args_ptr->packet_action )
    {
        while ( d_ptr < end_ptr )
        {
            //read the cmd_id
            c_api_get_u_short_be(d_ptr, &cmd_id);

            //read the cmd_len
            c_api_get_u_short_be(d_ptr, &cmd_len);

            switch ( cmd_id )
            {
                case C_API_CMD_REMOTE_LED_ID:
                {
                    handle_remote_led_cmd(d_ptr, cmd_len);
                    break;
                }// end case C_API_CMD_REMOTE_LED_ID:
                case C_API_CMD_MUTE_ID:
                {
                    handle_mute_cmd(d_ptr, cmd_len);
                    break;
                }// end case C_API_CMD_MUTE_ID:
                default:
                    break;
            }// end switch ( cmd_id )
            //jump over the previous command
            d_ptr = d_ptr+cmd_len;
        }// end while ( d_ptr < end_ptr )
    }// end if ( C_API_PACKET_ACTION_CMD == p_args_ptr->packet_action )
}// end <c_api_control_cmd_tag>




