#include <stdlib.h>
#include <xccompat.h>
#include <print.h>
#include "avb_conf.h"
#include "avb_1722_common.h"
#include "avb_1722_maap.h"
#include "avb_1722_maap_protocol.h"
#include "avb_control_types.h"
#include "ethernet_tx_client.h"
#include "misc_timer.h"
#include "nettypes.h"
#include "random.h"

// Macro that defines a constructor to initiliase the random number generator at startup
RANDOM_SIMPLE_INIT_SEED

typedef enum {
  MAAP_DISABLED,
  MAAP_PROBING,
  MAAP_RESERVED
} maap_state_t;

typedef struct {
  unsigned char base[6];
  int  range;
  int  probe_count;
  int  timeout;
  int immediately;
  maap_state_t state;
} maap_address_range;

static unsigned char my_mac_addr[6];
static unsigned char maap_dest_addr[6] = MAAP_PROTOCOL_DEST_ADDR;

static maap_address_range maap_addr =
{
  .base = MAAP_ALLOCATION_POOL_BASE_ADDR
};

static avb_timer maap_timer;
static int timeout_val;

static unsigned int maap_buf[(MAX_AVB_1722_MAAP_PDU_SIZE+1)/4];

static int create_maap_packet(int message_type,
                              unsigned char src_addr[6],
                              maap_address_range *addr,
                              char *buf,
                              unsigned char *conflict_addr,
                              int conflict_count)
{ 
  struct ethernet_hdr_t *hdr = (ethernet_hdr_t*) &buf[0];
  struct maap_packet_t *pkt = (maap_packet_t*) (hdr + 1);
  
  hdr->ethertype[0] = AVB_1722_ETHERTYPE >> 8;
  hdr->ethertype[1] = AVB_1722_ETHERTYPE & 0xff;
  
  SET_MAAP_CD_FLAG(pkt, DEFAULT_MAAP_CD_FLAG);
  SET_MAAP_SUBTYPE(pkt, DEFAULT_MAAP_SUBTYPE);
  SET_MAAP_AVB_VERSION(pkt, DEFAULT_MAAP_AVB_VERSION);
  SET_MAAP_VERSION(pkt, DEFAULT_MAAP_VERSION);
  SET_MAAP_DATALENGTH(pkt, sizeof(maap_packet_t));
  
  SET_MAAP_MSG_TYPE(pkt, message_type);
  for (int i=0; i < 6; i++)
  {
    hdr->src_addr[i] = my_mac_addr[i];
    pkt->request_start_address[i] = addr->base[i];
  }

  SET_MAAP_REQUESTED_COUNT(pkt, addr->range);

  if (message_type == MAAP_DEFEND)
  {
    for (int i=0; i < 6; i++)
    {
      pkt->conflict_start_address[i] = conflict_addr[i];
      hdr->dest_addr[i] = src_addr[i];
    }
    
    SET_MAAP_CONFLICT_COUNT(pkt, conflict_count);
  }
  else
  {
    for (int i=0; i < 6; i++)
    {
      pkt->conflict_start_address[i] = 0;
      hdr->dest_addr[i] = maap_dest_addr[i];
    }
    
    SET_MAAP_CONFLICT_COUNT(pkt, 0);
  }
  return (64);
}

void avb_1722_maap_init(unsigned char macaddr[6]) 
{
  maap_addr.state = MAAP_DISABLED;

  for (int i=0;i<6;i++)
  {
    my_mac_addr[i] = macaddr[i];
  }

  randomSimpleSeed((macaddr[3]<<24)+(macaddr[4]<<8)+(macaddr[5]));
  randomSimpleRandomiseSeed();

  init_avb_timer(&maap_timer, 1);
}

void avb_1722_maap_request_addresses(int num_addr, char start_address[]) 
{
  int offset;

  if (num_addr != -1)
  {
    maap_addr.range = num_addr;
  }
  // Set the base address randomly in the allocated maap address range
  offset = randomSimple() % (MAAP_ALLOCATION_POOL_SIZE - maap_addr.range);
  
  maap_addr.base[4] = (offset >> 8) & 0xFF;
  maap_addr.base[5] = offset & 0xFF;

  maap_addr.state = MAAP_PROBING;
  maap_addr.probe_count = MAAP_PROBE_RETRANSMITS;
  maap_addr.immediately = 1;

  timeout_val = MAAP_PROBE_INTERVAL_BASE_CS+(maap_addr.base[5]&7);
#if AVB_DEBUG_MAAP
  printstr("MAAP: Set probe interval ");
  printintln(timeout_val*10);
#endif
  start_avb_timer(&maap_timer, timeout_val);
}


void avb_1722_maap_rerequest_addresses() 
{
  if (maap_addr.state != MAAP_DISABLED)
  {
    maap_addr.state = MAAP_PROBING;
    maap_addr.probe_count = MAAP_PROBE_RETRANSMITS;
    maap_addr.immediately = 1;
  }
}

void avb_1722_maap_relinquish_addresses()
{
	maap_addr.state = MAAP_DISABLED;
}

void avb_1722_maap_get_offset_address(unsigned char addr[6], int offset)
{  
  // TODO: FIXME
  /*
  long long base = mac_addr_to_num(maap_addr.base);

  num_to_mac_addr(addr, base + offset);
  */

}

void avb_1722_maap_get_base_address(unsigned char addr[6])
{
  avb_1722_maap_get_offset_address(addr, 0);
}

void avb_1722_maap_periodic(avb_status_t *status, chanend c_tx)
{
  int nbytes;

  switch (maap_addr.state) 
  {
  case MAAP_DISABLED:
    break;
  case MAAP_PROBING:
    if (maap_addr.immediately || avb_timer_expired(&maap_timer))
    {
      maap_addr.immediately = 0;

      nbytes = create_maap_packet(MAAP_PROBE, NULL, &maap_addr, (char *) &maap_buf[0], NULL, 0);
      mac_tx(c_tx, maap_buf, nbytes, ETH_BROADCAST);
      maap_addr.probe_count--;

      if (maap_addr.probe_count == 0)
      {
        maap_addr.state = MAAP_RESERVED;
        maap_addr.immediately = 1;
        timeout_val = MAAP_ANNOUNCE_INTERVAL_BASE_CS + (maap_addr.base[5]&0x1F);
      #if AVB_DEBUG_MAAP
        printstr("MAAP: Set announce interval ");
        printintln(timeout_val*10);
      #endif

        init_avb_timer(&maap_timer, MAAP_ANNOUNCE_INTERVAL_MULTIPLIER);
        start_avb_timer(&maap_timer, timeout_val);

        // 5.2 TODO: Do event AVB_MAAP_ADDRESSES_RESERVED
      }
      else
      {
        // reset timeout
        start_avb_timer(&maap_timer, timeout_val);
      }
    }
    break;
  case MAAP_RESERVED:
    if (maap_addr.immediately || avb_timer_expired(&maap_timer))
    {
      nbytes = create_maap_packet(MAAP_ANNOUNCE, NULL, &maap_addr, (char *) &maap_buf[0], NULL, 0);
      mac_tx(c_tx, maap_buf, nbytes, ETH_BROADCAST);

      if (!maap_addr.immediately)
      {
        // reset timeout
        start_avb_timer(&maap_timer, timeout_val);
      }
      else
      {
        maap_addr.immediately = 0;
      }
    }
    break;
  } 
  return;
}

static int maap_compare_mac(unsigned char src_addr[6])
{
  for (int i=0; i < 6; i++)
  {
    if (my_mac_addr[5-i] < src_addr[5-i]) return 1;
  }
  return 0;
}

static int maap_conflict(unsigned char remote_addr[6], int remote_count, unsigned char conflicted_addr[6], int *conflicted_count)
{
  int my_addr_lo;
  int my_addr_hi;
  int conflict_lo;
  int conflict_hi;
  int first_conflict_addr;
  
  // First, check the address is within the IEEE allocation pool
  for (int i=0; i < 4; i++)
  {
    if (remote_addr[i] != maap_addr.base[i]) return 0;
  }

  my_addr_lo = (int)maap_addr.base[5] + ((int)maap_addr.base[4]<<8);
  my_addr_hi = my_addr_lo + maap_addr.range;

  conflict_lo = (int)remote_addr[5] + ((int)remote_addr[4]<<8);
  conflict_hi = conflict_lo + remote_count;

  // We need to find the "first allocated address that conflicts with the requested address range"
  // to fill the Defend packet
  if ((my_addr_lo >= conflict_lo) && (my_addr_lo < conflict_hi))
  {
    first_conflict_addr = my_addr_lo;

    if (conflicted_count != NULL)
    {
      if (my_addr_hi < conflict_hi)
      {
        *conflicted_count = my_addr_hi - first_conflict_addr;
      }
      else
      {
        *conflicted_count = conflict_hi - first_conflict_addr;
      }
    }
    else
    {
      return 1;
    }
  }
  else if ((conflict_lo > my_addr_lo) && (conflict_lo < my_addr_hi))
  {
    first_conflict_addr = my_addr_lo + (conflict_lo - my_addr_lo);

    if (conflicted_count != NULL)
    {
      if (conflict_hi <= my_addr_hi)
      {
        *conflicted_count = remote_count;
      }
      else
      {
        *conflicted_count = my_addr_hi - first_conflict_addr;
      }
    }
    else
    {
      return 1;
    }
  }
  else // No conflict
  {
    return 0;
  }
  
  // We have a conflict.

  if (maap_addr.state == MAAP_RESERVED)
  {
    // We are in the MAAP_RESERVED (DEFEND) state and received a Probe message
    // Fill in the conflict_start_address field

    for (int i=0; i<4; i++)
    {
      conflicted_addr[i] = maap_addr.base[i];        
    }

    conflicted_addr[4] = (unsigned char)(first_conflict_addr >> 8);
    conflicted_addr[5] = (unsigned char)first_conflict_addr;
  }
  
  return 1;
}

void avb_1722_maap_process_packet(avb_status_t *status, unsigned int buf0[], unsigned char src_addr[6], int nbytes, chanend c_tx)
{
  unsigned char *buf = (unsigned char *) buf0;
  struct maap_packet_t *maap_pkt = (struct maap_packet_t *) &buf[0];
  int msg_type;
  unsigned char conflict_addr[6];
  int conflict_count;
  unsigned char *test_addr;
  int test_count;

  if (GET_MAAP_SUBTYPE(maap_pkt) != 0x7e)
  {
    // not a MAAP packet
    return;
  }

  if (maap_addr.state == MAAP_DISABLED)
    return;

  msg_type = GET_MAAP_MSG_TYPE(maap_pkt);
  test_addr = &(maap_pkt->request_start_address[0]);
  test_count = GET_MAAP_REQUESTED_COUNT(maap_pkt);

  switch (msg_type)
  {
  case MAAP_PROBE:
    if (maap_conflict(test_addr, test_count, conflict_addr, &conflict_count))
    {
      if (maap_addr.state == MAAP_PROBING)
      {
        if (maap_compare_mac(src_addr)) return;
        // Generate new addresses using the same range count as before:
        avb_1722_maap_request_addresses(-1, NULL); 
      }
      else
      {
        int len;
        len = create_maap_packet(MAAP_DEFEND, src_addr, &maap_addr, (char*) &maap_buf[0], conflict_addr, conflict_count);
        mac_tx(c_tx, maap_buf, len, ETH_BROADCAST);
      }
    }
    break;
  case MAAP_DEFEND:
    test_addr = &maap_pkt->conflict_start_address[0];
    test_count = GET_MAAP_CONFLICT_COUNT(maap_pkt);
    /* Fallthrough intentional */
  case MAAP_ANNOUNCE:
    if (maap_conflict(test_addr, test_count, NULL, NULL))
    {
      if (maap_addr.state == MAAP_RESERVED)
      {
        if (maap_compare_mac(src_addr)) return;
      }

      // Restart the state machine using the same range count as before:
      avb_1722_maap_request_addresses(-1, NULL);

      return;
    }
    break;
  }

  return;
}

