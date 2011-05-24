#include "xccompat.h"
#include "avb_conf.h"
#include "avb_1722_common.h"
#include "avb_1722_1.h"
#include "avb_1722_1_protocol.h"
#include "ethernet_tx_client.h"
#include "nettypes.h"
#include "misc_timer.h"
#include "print.h"

static unsigned char my_mac_addr[6];

static unsigned char avb_1722_1_sdp_dest_addr[6] =  {0xff, 0xff, 0xff, 0xff, 0xff, 0xff};
//static unsigned char avb_1722_1_scm_dest_addr[6] =  {0xff, 0xff, 0xff, 0xff, 0xff, 0xff};
//static unsigned char avb_1722_1_sec_dest_addr[6] =  {0xff, 0xff, 0xff, 0xff, 0xff, 0xff};

static unsigned int avb_1722_1_buf[(sizeof(avb_1722_1_packet_t)+sizeof(ethernet_hdr_t)+3)/4];

static avb_timer sdp_timer;

enum { IDLE, ADVERTISE_1, ADVERTISE_2, WAITING, DEPARTING_1, DEPARTING_2 } sdp_advertise_state = IDLE;

void avb_1722_1_init(unsigned char macaddr[6])
{
  for (int i=0;i<6;i++)
    my_mac_addr[i] = macaddr[i];

  init_avb_timer(&sdp_timer, 1);
}

static void avb_1722_1_create_1722_1_header(unsigned char* dest_addr, int subtype, int message_type, char valid_time_status, unsigned data_len, ethernet_hdr_t *hdr)
{
	struct avb_1722_1_packet_header_t *pkt = (struct avb_1722_1_packet_header_t *) (hdr + 1);

	for (int i=0;i<6;i++) {
	  hdr->src_addr[i] = my_mac_addr[i];
	  hdr->dest_addr[i] = dest_addr[i];
	}

	hdr->ethertype[0] = AVB_ETYPE >> 8;
	hdr->ethertype[1] = AVB_ETYPE & 0xff;

	SET_1722_1_CD_FLAG(pkt, 1);
	SET_1722_1_SUBTYPE(pkt, subtype);
	SET_1722_1_SV(pkt, 0);
	SET_1722_1_AVB_VERSION(pkt, 0);
	SET_1722_1_MSG_TYPE(pkt, message_type);
	SET_1722_1_VALID_TIME(pkt, valid_time_status);
	SET_1722_1_DATALENGTH(pkt, data_len);
}


static void avb_1722_1_create_sdp_packet(int message_type)
{
	  ethernet_hdr_t *hdr = (ethernet_hdr_t*) &avb_1722_1_buf[0];
	  avb_1722_1_sdp_packet_t *pkt = (avb_1722_1_sdp_packet_t*) (hdr + 1);

	  avb_1722_1_create_1722_1_header(avb_1722_1_sdp_dest_addr, DEFAULT_1722_1_SDP_SUBTYPE, message_type,
			  (message_type==ENTITY_AVAILABLE)?AVB_1722_1_SDP_VALID_TIME:0, 40, hdr);

	  SET_WORD(pkt->entity_guid_lo, AVB_1722_1_SDP_ENTITY_GUID_LO);
	  SET_WORD(pkt->entity_guid_hi, AVB_1722_1_SDP_ENTITY_GUID_HI);
	  SET_WORD(pkt->vendor_id, AVB_1722_1_SDP_VENDOR_ID);
	  SET_WORD(pkt->model_id, AVB_1722_1_SDP_MODEL_ID);
	  SET_WORD(pkt->entity_capabilities, AVB_1722_1_SDP_ENTITY_CAPIBILITIES);
	  pkt->talker_stream_sources = AVB_1722_1_SDP_TALKER_STREAM_SOURCES;
	  pkt->talker_capabilities = AVB_1722_1_SDP_TALKER_CAPABILITIES;
	  pkt->listener_stream_sinks = AVB_1722_1_SDP_LISTENER_STREAM_SINKS;
	  pkt->listener_capabilites = AVB_1722_1_SDP_LISTENER_CAPIBILITIES;
	  SET_WORD(pkt->controller_capabilities, AVB_1722_1_SDP_CONTROLLER_CAPABILITIES);
	  SET_WORD(pkt->boot_id, AVB_1722_1_SDP_BOOT_ID);
	  pkt->reserved[0] = 0;
	  pkt->reserved[1] = 0;
	  pkt->reserved[2] = 0;
	  pkt->reserved[3] = 0;
	  pkt->reserved[4] = 0;
	  pkt->reserved[5] = 0;
}

/*
static void avb_1722_1_create_scm_packet(int message_type)
{
	  struct ethernet_hdr_t *hdr = (ethernet_hdr_t*) &avb_1722_1_buf[0];
	  struct avb_1722_1_scm_packet_t *pkt = (avb_1722_1_scm_packet_t*) (hdr + 1);

	  avb_1722_1_create_1722_1_header(avb_1722_1_scm_dest_addr, DEFAULT_1722_1_SCM_SUBTYPE, message_type, 0, 40, hdr);
}

static void avb_1722_1_create_sec_packet(int message_type)
{
	  struct ethernet_hdr_t *hdr = (ethernet_hdr_t*) &avb_1722_1_buf[0];
	  struct avb_1722_1_sec_packet_t *pkt = (avb_1722_1_sec_packet_t*) (hdr + 1);

	  avb_1722_1_create_1722_1_header(avb_1722_1_sec_dest_addr, DEFAULT_1722_1_SEC_SUBTYPE, message_type, 0, 40, hdr);
}
*/

avb_status_t process_avb_1722_1_sdp_packet(avb_1722_1_sdp_packet_t* pkt)
{
	// If the packet is querying all or me then
	sdp_advertise_state = ADVERTISE_1;
	return AVB_1722_1_OK;
}

avb_status_t process_avb_1722_1_sec_packet(avb_1722_1_sec_packet_t* pkt)
{
	return AVB_1722_1_OK;
}

avb_status_t process_avb_1722_1_scm_packet(avb_1722_1_scm_packet_t* pkt)
{
	return AVB_1722_1_OK;
}

avb_status_t avb_1722_1_process_packet(unsigned int buf0[], int len, chanend c_tx)
{
	  unsigned char *buf = (unsigned char *) buf0;

	  struct ethernet_hdr_t *ethernet_hdr = (ethernet_hdr_t *) &buf[0];
	  struct tagged_ethernet_hdr_t *tagged_ethernet_hdr =
	    (tagged_ethernet_hdr_t *) &buf[0];

	  int has_qtag = ethernet_hdr->ethertype[1]==0x18;
	  int ethernet_pkt_size = has_qtag ? 18 : 14;

	  struct avb_1722_1_packet_header_t *pkt =
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


	  if (GET_1722_1_CD_FLAG(pkt) != 1)
		    // not a 1722.1 packet
		    return AVB_NO_STATUS;

	  {
		  unsigned subtype = GET_1722_1_SUBTYPE(pkt);
		  switch (subtype)
		  {
		  case DEFAULT_1722_1_SDP_SUBTYPE:
			  return process_avb_1722_1_sdp_packet((avb_1722_1_sdp_packet_t*)pkt);
		  case DEFAULT_1722_1_SEC_SUBTYPE:
			  return process_avb_1722_1_sec_packet((avb_1722_1_sec_packet_t*)pkt);
		  case DEFAULT_1722_1_SCM_SUBTYPE:
			  return process_avb_1722_1_scm_packet((avb_1722_1_scm_packet_t*)pkt);
		  default:
			  return AVB_NO_STATUS;
		  }
	  }

	  return AVB_1722_1_OK;
}

static void avb_1722_1_sdp_advertising_periodic(chanend c_tx)
{
	switch (sdp_advertise_state) {
	case IDLE:
		break;
	case ADVERTISE_1:
		avb_1722_1_create_sdp_packet(ENTITY_AVAILABLE);
		mac_tx(c_tx, avb_1722_1_buf, 60, ETH_BROADCAST);
		printstr("Sending SDP packet 1\n");
		start_avb_timer(&sdp_timer, 3); // 3 centiseconds
		sdp_advertise_state = ADVERTISE_2;
		break;
	case ADVERTISE_2:
		if (avb_timer_expired(&sdp_timer)) {
			avb_1722_1_create_sdp_packet(ENTITY_AVAILABLE);
			mac_tx(c_tx, avb_1722_1_buf, 60, ETH_BROADCAST);
			printstr("Sending SDP packet 2\n");
			sdp_advertise_state = WAITING;
		}
		break;
	case WAITING:
		break;
	case DEPARTING_1:
		avb_1722_1_create_sdp_packet(ENTITY_DEPARTING);
		mac_tx(c_tx, avb_1722_1_buf, 60, ETH_BROADCAST);
		start_avb_timer(&sdp_timer, 3); // 3 centiseconds
		sdp_advertise_state = DEPARTING_2;
		break;
	case DEPARTING_2:
		if (avb_timer_expired(&sdp_timer)) {
			avb_1722_1_create_sdp_packet(ENTITY_DEPARTING);
			mac_tx(c_tx, avb_1722_1_buf, 60, ETH_BROADCAST);
			sdp_advertise_state = IDLE;
		}
		break;
	default:
		break;
	}
}

void avb_1722_1_periodic(chanend c_tx)
{
	avb_1722_1_sdp_advertising_periodic(c_tx);
}


void avb_1722_1_sdp_announce()
{
	if (sdp_advertise_state == IDLE) sdp_advertise_state = ADVERTISE_1;
}


void avb_1722_1_sdp_depart()
{
	if (sdp_advertise_state != IDLE) sdp_advertise_state = DEPARTING_1;
}
