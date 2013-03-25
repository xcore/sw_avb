#include <xccompat.h>
#include <print.h>
#include "simple_printf.h"
#include "avb.h"
#include "avb_conf.h"
#include "avb_1722_common.h"
#include "avb_control_types.h"
#include "avb_1722_1_common.h"
#include "avb_1722_1_aecp.h"
#include "aem_descriptor_types.h"
#include "app_config.h"
#include "avb_1722_1_acmp.h"
#include "avb_1722_1_adp.h"



static unsigned control_value_changes = 0;

static unsigned char aem_identify_control_value = 0;

unsigned short avb_entity_set_control_value(unsigned short control_type, unsigned short control_index, unsigned short values_length, unsigned char values[510])
{
	unsigned short result = AECP_AEM_STATUS_NO_SUCH_DESCRIPTOR;
	
	simple_printf("aem_control: Setting Control type %x index %x value length %u\n", control_type, control_index, values_length);
  	
  	if(AEM_CONTROL_TYPE == control_type)
  	{
  		switch(control_index)
  		{
  			case DESCRIPTOR_INDEX_CONTROL_IDENTIFY:
  				if(1 == values_length)
  				{
  					aem_identify_control_value = values[0];
  					control_value_changes++;
  					result = AECP_AEM_STATUS_SUCCESS;
  				}
  				else
  				{
  					result = AECP_AEM_STATUS_BAD_ARGUMENTS;
  				}
  				break;
  		}
  	}
	
	return result;
}

unsigned short avb_entity_get_control_value(unsigned short control_type, unsigned short control_index, unsigned short *values_length, unsigned char values[510])
{
	unsigned short result = AECP_AEM_STATUS_NO_SUCH_DESCRIPTOR;
	
	simple_printf("aem_control: Getting Control type %x index %x value\n", control_type, control_index);
	
  	if(AEM_CONTROL_TYPE == control_type)
  	{
  		switch(control_index)
  		{
  			case DESCRIPTOR_INDEX_CONTROL_IDENTIFY:
  				*values_length = 1;
  				values[0] = aem_identify_control_value;
				result = AECP_AEM_STATUS_SUCCESS;
				break;
  		}
  	}
	
	return result;
}

unsigned char get_aem_identify_control_value(void)
{
	return aem_identify_control_value;
}


/* The controller has indicated to connect this listener sink to a talker stream */
void avb_listener_on_talker_connect(int sink_num, guid_t *talker_guid, unsigned char dest_addr[6], unsigned int stream_id[2], guid_t *my_guid)
{
  int map[AVB_NUM_MEDIA_OUTPUTS];
  for (int i = 0; i < AVB_NUM_MEDIA_OUTPUTS; i++) map[i] = i;

  simple_printf("CONNECTING Listener sink #%d -> Talker stream %x%x, DA: ", sink_num, stream_id[0], stream_id[1]); print_mac_ln(dest_addr);

  set_device_media_clock_type(0, DEVICE_MEDIA_CLOCK_INPUT_STREAM_DERIVED);

  set_avb_sink_sync(sink_num, 0);
  set_avb_sink_channels(sink_num, AVB_NUM_MEDIA_OUTPUTS);
  set_avb_sink_map(sink_num, map, AVB_NUM_MEDIA_OUTPUTS);
  set_avb_sink_id(sink_num, stream_id);
  set_avb_sink_addr(sink_num, dest_addr, 6);

  set_avb_sink_state(sink_num, AVB_SINK_STATE_POTENTIAL);

}

void avb_talker_on_source_address_reserved(int source_num, unsigned char mac_addr[6])
{
  // Do some debug print
  simple_printf("MAAP reserved Talker stream #%d address: %x:%x:%x:%x:%x:%x\n", source_num,
                            mac_addr[0],
                            mac_addr[1],
                            mac_addr[2],
                            mac_addr[3],
                            mac_addr[4],
                            mac_addr[5]);

  set_avb_source_dest(source_num, mac_addr, 6);

  /* NOTE: acmp_talker_init() must be called BEFORE talker_set_mac_address() otherwise it will zero
   * what was just set */
  avb_1722_1_acmp_talker_init();
  avb_1722_1_talker_set_mac_address(source_num, mac_addr);
  avb_1722_1_adp_announce();

}
