#include "xccompat.h"
#include "avb_1722_common.h"
#include "avb_1722_1.h"
#include "avb_1722_1_protocol.h"
#include "ethernet_tx_client.h"
#include "nettypes.h"



void avb_1722_1_init()
{

}

/*
static void avb_1722_1_create_packet()
{

}
*/

void avb_1722_1_periodic(chanend c_tx)
{

}

avb_status_t avb_1722_1_process_packet(unsigned int buf0[], int len, chanend c_tx)
{
	  unsigned char *buf = (unsigned char *) buf0;

	  struct ethernet_hdr_t *ethernet_hdr = (ethernet_hdr_t *) &buf[0];
	  struct tagged_ethernet_hdr_t *tagged_ethernet_hdr =
	    (tagged_ethernet_hdr_t *) &buf[0];

	  int has_qtag = ethernet_hdr->ethertype[1]==0x18;
	  int ethernet_pkt_size = has_qtag ? 18 : 14;

	  struct avb_1722_1_packet_header_t *avb_1722_1_pkt =
	    (struct avb_1722_1_packet_header_t *) &buf[ethernet_pkt_size];

	  if (has_qtag) {
	    if (tagged_ethernet_hdr->ethertype[1] != (AVB_ETYPE & 0xff) ||
	        tagged_ethernet_hdr->ethertype[0] != (AVB_ETYPE >> 8))
	      {
	        // not a 1722 packet
	        return AVB_NO_STATUS;
	      }
	  }
	  else {
	    if (ethernet_hdr->ethertype[1] != (AVB_ETYPE & 0xff) ||
	        ethernet_hdr->ethertype[0] != (AVB_ETYPE >> 8))
	      {
	        // not a 1722 packet
	        return AVB_NO_STATUS;
	      }
	  }


	  if (GET_1722_1_CD_FLAG(avb_1722_1_pkt) != 1 ||
		      GET_1722_1_SUBTYPE(avb_1722_1_pkt) != 0x7a)
		    // not a 1722.1 packet
		    return AVB_NO_STATUS;

	  return AVB_1722_1_OK;
}
