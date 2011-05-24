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

static avb_timer sdp_advertise_timer;
static avb_timer sdp_discovery_timer;

static unsigned sdp_two_second_counter = 0;
static unsigned my_guid[2];
static unsigned discover_guid[2];

enum { SDP_ADVERTISE_IDLE,
	   SDP_ADVERTISE_ADVERTISE_1,
	   SDP_ADVERTISE_ADVERTISE_2,
	   SDP_ADVERTISE_WAITING,
	   SDP_ADVERTISE_DEPARTING_1,
	   SDP_ADVERTISE_DEPARTING_2
} sdp_advertise_state = SDP_ADVERTISE_IDLE;

enum { SDP_DISCOVERY_IDLE,
	   SDP_DISCOVERY_WAITING,
	   SDP_DISCOVERY_DISCOVER
} sdp_discovery_state = SDP_DISCOVERY_IDLE;

enum { SCM_TALKER_IDLE,
	   SCM_TALKER_WAITING
} scm_talker_state = SCM_TALKER_IDLE;

enum { SCM_LISTENER_IDLE,
	   SCM_LISTENER_WAITING
} scm_listener_state = SCM_LISTENER_IDLE;


void avb_1722_1_init(unsigned char macaddr[6])
{
  for (int i=0;i<6;i++)
    my_mac_addr[i] = macaddr[i];

  my_guid[0] = AVB_1722_1_SDP_ENTITY_GUID_LO;
  my_guid[1] = AVB_1722_1_SDP_ENTITY_GUID_HI;

  init_avb_timer(&sdp_advertise_timer, 1);
  init_avb_timer(&sdp_discovery_timer, 1);
}

static void avb_1722_1_entity_database_add(avb_1722_1_sdp_packet_t* pkt)
{
	printstr("1722.1 entity database: added\n");
}

static void avb_1722_1_entity_database_remove(avb_1722_1_sdp_packet_t* pkt)
{
	printstr("1722.1 entity database: added\n");
}

static void avb_1722_1_entity_database_check_timeout()
{

}


static void avb_1722_1_create_1722_1_header(unsigned char* dest_addr, int subtype, int message_type, char valid_time_status, unsigned data_len, ethernet_hdr_t *hdr)
{
	avb_1722_1_packet_header_t *pkt = (avb_1722_1_packet_header_t *) (hdr + 1);

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


static void avb_1722_1_create_sdp_packet(int message_type, unsigned guid[2])
{
	  ethernet_hdr_t *hdr = (ethernet_hdr_t*) &avb_1722_1_buf[0];
	  avb_1722_1_sdp_packet_t *pkt = (avb_1722_1_sdp_packet_t*) (hdr + 1);

	  avb_1722_1_create_1722_1_header(avb_1722_1_sdp_dest_addr, DEFAULT_1722_1_SDP_SUBTYPE, message_type,
			  (message_type==ENTITY_AVAILABLE)?AVB_1722_1_SDP_VALID_TIME:0, 40, hdr);

	  SET_WORD(pkt->entity_guid_lo, guid[0]);
	  SET_WORD(pkt->entity_guid_hi, guid[1]);

	  if (message_type==ENTITY_DISCOVER)
	  {
		  pkt->vendor_id[0] = 0;
		  pkt->vendor_id[1] = 0;
		  pkt->model_id[0] = 0;
		  pkt->model_id[1] = 0;
		  pkt->entity_capabilities[0] = 0;
		  pkt->entity_capabilities[1] = 0;
		  pkt->talker_stream_sources = 0;
		  pkt->talker_capabilities = 0;
		  pkt->listener_stream_sinks = 0;
		  pkt->listener_capabilites = 0;
		  pkt->controller_capabilities[0] = 0;
		  pkt->controller_capabilities[1] = 0;
		  pkt->boot_id[0] = 0;
		  pkt->boot_id[1] = 0;
	  }
	  else
	  {
		  SET_WORD(pkt->vendor_id, AVB_1722_1_SDP_VENDOR_ID);
		  SET_WORD(pkt->model_id, AVB_1722_1_SDP_MODEL_ID);
		  SET_WORD(pkt->entity_capabilities, AVB_1722_1_SDP_ENTITY_CAPIBILITIES);
		  pkt->talker_stream_sources = AVB_1722_1_SDP_TALKER_STREAM_SOURCES;
		  pkt->talker_capabilities = AVB_1722_1_SDP_TALKER_CAPABILITIES;
		  pkt->listener_stream_sinks = AVB_1722_1_SDP_LISTENER_STREAM_SINKS;
		  pkt->listener_capabilites = AVB_1722_1_SDP_LISTENER_CAPIBILITIES;
		  SET_WORD(pkt->controller_capabilities, AVB_1722_1_SDP_CONTROLLER_CAPABILITIES);
		  SET_WORD(pkt->boot_id, AVB_1722_1_SDP_BOOT_ID);
	  }
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
	avb_1722_1_packet_header_t* hdr = (avb_1722_1_packet_header_t*)pkt;

	if (GET_1722_1_MSG_TYPE(hdr)==ENTITY_DISCOVER)
	{
		if (COMPARE_WORD(pkt->entity_guid_lo, AVB_1722_1_SDP_ENTITY_GUID_LO) ||
			COMPARE_WORD(pkt->entity_guid_lo, 0))
		{
			sdp_advertise_state = SDP_ADVERTISE_ADVERTISE_1;
		}
		return AVB_1722_1_OK;
	}

	if (GET_1722_1_MSG_TYPE(hdr)==ENTITY_AVAILABLE)
	{
		avb_1722_1_entity_database_add(pkt);
		return AVB_1722_1_ENTITY_ADDED;
	}

	if (GET_1722_1_MSG_TYPE(hdr)==ENTITY_DEPARTING)
	{
		avb_1722_1_entity_database_remove(pkt);
		return AVB_1722_1_ENTITY_REMOVED;
	}

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

static void avb_1722_1_scm_talker_periodic(chanend c_tx)
{
	switch (scm_talker_state)
	{
	case SCM_TALKER_IDLE:
		break;
	case SCM_TALKER_WAITING:
		break;
	}
}

static void avb_1722_1_scm_listener_periodic(chanend c_tx)
{
	switch (scm_listener_state)
	{
	case SCM_LISTENER_IDLE:
		break;
	case SCM_LISTENER_WAITING:
		break;
	}
}

static void avb_1722_1_sdp_discovery_periodic(chanend c_tx)
{
	switch (sdp_discovery_state)
	{
	case SDP_DISCOVERY_IDLE:
		break;
	case SDP_DISCOVERY_WAITING:
		{
			if (avb_timer_expired(&sdp_discovery_timer)) {
				sdp_two_second_counter++;
				avb_1722_1_entity_database_check_timeout();
				start_avb_timer(&sdp_discovery_timer, 200);
			}
		}
		break;
	case SDP_DISCOVERY_DISCOVER:
		{
			avb_1722_1_create_sdp_packet(ENTITY_DISCOVER, discover_guid);
			mac_tx(c_tx, avb_1722_1_buf, 60, ETH_BROADCAST);
			sdp_discovery_state = SDP_DISCOVERY_WAITING;
		}
		break;
	}
}

static void avb_1722_1_sdp_advertising_periodic(chanend c_tx)
{
	switch (sdp_advertise_state) {
	case SDP_ADVERTISE_IDLE:
		break;
	case SDP_ADVERTISE_ADVERTISE_1:
		avb_1722_1_create_sdp_packet(ENTITY_AVAILABLE, my_guid);
		mac_tx(c_tx, avb_1722_1_buf, 60, ETH_BROADCAST);
		printstr("Sending SDP packet 1\n");
		start_avb_timer(&sdp_advertise_timer, 3); // 3 centiseconds
		sdp_advertise_state = SDP_ADVERTISE_ADVERTISE_2;
		break;
	case SDP_ADVERTISE_ADVERTISE_2:
		if (avb_timer_expired(&sdp_advertise_timer)) {
			avb_1722_1_create_sdp_packet(ENTITY_AVAILABLE, my_guid);
			mac_tx(c_tx, avb_1722_1_buf, 60, ETH_BROADCAST);
			sdp_advertise_state = SDP_ADVERTISE_WAITING;
		}
		break;
	case SDP_ADVERTISE_WAITING:
		break;
	case SDP_ADVERTISE_DEPARTING_1:
		avb_1722_1_create_sdp_packet(ENTITY_DEPARTING, my_guid);
		mac_tx(c_tx, avb_1722_1_buf, 60, ETH_BROADCAST);
		start_avb_timer(&sdp_advertise_timer, 3); // 3 centiseconds
		sdp_advertise_state = SDP_ADVERTISE_DEPARTING_2;
		break;
	case SDP_ADVERTISE_DEPARTING_2:
		if (avb_timer_expired(&sdp_advertise_timer)) {
			avb_1722_1_create_sdp_packet(ENTITY_DEPARTING, my_guid);
			mac_tx(c_tx, avb_1722_1_buf, 60, ETH_BROADCAST);
			sdp_advertise_state = SDP_ADVERTISE_IDLE;
		}
		break;
	default:
		break;
	}
}

void avb_1722_1_periodic(chanend c_tx)
{
	avb_1722_1_sdp_advertising_periodic(c_tx);
	avb_1722_1_sdp_discovery_periodic(c_tx);
	avb_1722_1_scm_listener_periodic(c_tx);
	avb_1722_1_scm_talker_periodic(c_tx);
}


void avb_1722_1_sdp_announce()
{
	if (sdp_advertise_state == SDP_ADVERTISE_IDLE) sdp_advertise_state = SDP_ADVERTISE_ADVERTISE_1;
}


void avb_1722_1_sdp_depart()
{
	if (sdp_advertise_state != SDP_ADVERTISE_IDLE) sdp_advertise_state = SDP_ADVERTISE_DEPARTING_1;
}

void avb_1722_1_sdp_discover(unsigned guid[])
{
	if (sdp_discovery_state == SDP_DISCOVERY_WAITING) {
		sdp_discovery_state = SDP_DISCOVERY_DISCOVER;
		discover_guid[0] = guid[0];
		discover_guid[1] = guid[1];
	}
}

void avb_1722_1_sdp_discover_all()
{
	unsigned guid[2] = {0,0};
	avb_1722_1_sdp_discover(guid);
}
