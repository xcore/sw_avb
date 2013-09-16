#include "avb_1722_1_aecp_controls.h"
#include "avb.h"
#include "avb_api.h"
#include "avb_1722_1_common.h"
#include "avb_1722_1_aecp.h"
#include "misc_timer.h"
#include "avb_srp_pdu.h"
#include <string.h>
#include <print.h>
#include "simple_printf.h"
#include "xccompat.h"
#include "avb_1722_1.h"

#if AVB_1722_1_AEM_ENABLED
#include "aem_descriptor_types.h"
#endif

unsafe unsigned short process_aem_cmd_getset_control(avb_1722_1_aecp_packet_t *unsafe pkt,
                                                     unsigned char &status,
                                                     unsigned short command_type, 
                                                     client interface avb_1722_1_control_callbacks i_1722_1_entity)
{
  avb_1722_1_aem_getset_control_t *cmd = (avb_1722_1_aem_getset_control_t *)(pkt->data.aem.command.payload);
  unsigned short control_index = ntoh_16(cmd->descriptor_id);
  unsigned short control_type = ntoh_16(cmd->descriptor_type);
  unsigned char *values = pkt->data.aem.command.payload + sizeof(avb_1722_1_aem_getset_control_t);
  unsigned short values_length = GET_1722_1_DATALENGTH(&(pkt->header)) - sizeof(avb_1722_1_aem_getset_control_t) - AVB_1722_1_AECP_COMMAND_DATA_OFFSET; 

  if (command_type == AECP_AEM_CMD_GET_CONTROL)
  {
    status = i_1722_1_entity.get_control_value(control_type, control_index, values_length, values);
  }
  else // AECP_AEM_CMD_SET_CONTROL
  {
    simple_printf("Send %d\n", values_length);
    status = i_1722_1_entity.set_control_value(control_type, control_index, values_length, values);
  }
  return values_length;
}

unsafe void process_aem_cmd_getset_sampling_rate(avb_1722_1_aecp_packet_t *unsafe pkt,
                                          unsigned char &status,
                                          unsigned short command_type,
                                          client interface avb_interface avb)
{
  avb_1722_1_aem_getset_sampling_rate_t *cmd = (avb_1722_1_aem_getset_sampling_rate_t *)(pkt->data.aem.command.payload);
  unsigned short media_clock_id = ntoh_16(cmd->descriptor_id);
  int rate;

  if (command_type == AECP_AEM_CMD_GET_SAMPLING_RATE)
  {
    if (avb.get_device_media_clock_rate(media_clock_id, rate))
    {
      hton_32(cmd->sampling_rate, rate);
      return;
    }
  }
  else // AECP_AEM_CMD_SET_SAMPLING_RATE
  {
    rate = ntoh_32(cmd->sampling_rate);

    if (avb.set_device_media_clock_rate(media_clock_id, rate))
    {
      simple_printf("SET SAMPLING RATE TO %d\n", rate);
      // Success
      return;
    }
  }

  status = AECP_AEM_STATUS_NO_SUCH_DESCRIPTOR;
}

unsafe void process_aem_cmd_getset_clock_source(avb_1722_1_aecp_packet_t *unsafe pkt,
                                         unsigned char &status,
                                         unsigned short command_type,
                                         client interface avb_interface avb)
{
  avb_1722_1_aem_getset_clock_source_t *cmd = (avb_1722_1_aem_getset_clock_source_t *)(pkt->data.aem.command.payload);
  unsigned short media_clock_id = ntoh_16(cmd->descriptor_id);
  // The clock source descriptor's index corresponds to the clock type in our implementation
  enum device_media_clock_type_t source_index;

  if (command_type == AECP_AEM_CMD_GET_CLOCK_SOURCE)
  {
    if (avb.get_device_media_clock_type(media_clock_id, source_index))
    {
      hton_16(cmd->clock_source_index, source_index);
      return;
    }
  }
  else // AECP_AEM_CMD_SET_CLOCK_SOURCE
  {
    source_index = ntoh_16(cmd->clock_source_index);

    if (avb.set_device_media_clock_type(media_clock_id, source_index))
    {
      // Success
      return;
    }
  }

  status = AECP_AEM_STATUS_NO_SUCH_DESCRIPTOR;
}

unsafe void process_aem_cmd_startstop_streaming(avb_1722_1_aecp_packet_t *unsafe pkt,
                                         unsigned char &status,
                                         unsigned short command_type,
                                         client interface avb_interface avb)
{
  avb_1722_1_aem_startstop_streaming_t *cmd = (avb_1722_1_aem_startstop_streaming_t *)(pkt->data.aem.command.payload);
  unsigned short stream_index = ntoh_16(cmd->descriptor_id);
  unsigned short desc_type = ntoh_16(cmd->descriptor_type);

  if (desc_type == AEM_STREAM_INPUT_TYPE)
  {
    enum avb_sink_state_t state;
    if (avb.get_sink_state(stream_index, state))
    {
      if (command_type == AECP_AEM_CMD_START_STREAMING)
      {
        avb.set_sink_state(stream_index, AVB_SINK_STATE_ENABLED);
      }
      else
      {
        if (state == AVB_SINK_STATE_ENABLED)
        {
          avb.set_sink_state(stream_index, AVB_SINK_STATE_POTENTIAL);
        }
      }
    }
    else status = AECP_AEM_STATUS_NO_SUCH_DESCRIPTOR;

  }
  else if ((desc_type == AEM_STREAM_OUTPUT_TYPE))
  {
    enum avb_source_state_t state;
    if (avb.get_source_state(stream_index, state))
    {
      if (command_type == AECP_AEM_CMD_START_STREAMING)
      {
        avb.set_source_state(stream_index, AVB_SOURCE_STATE_ENABLED);
      }
      else
      {
        if (state == AVB_SINK_STATE_ENABLED)
        {
          avb.set_source_state(stream_index, AVB_SOURCE_STATE_POTENTIAL);
        }
      }
    }
    else status = AECP_AEM_STATUS_NO_SUCH_DESCRIPTOR;
  }
}
