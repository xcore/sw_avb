#include <stdlib.h>
#include "xccompat.h"
#include "avb_1722_common.h"
#include "avb_1722_maap.h"
#include "avb_1722_maap_protocol.h"
#include "avb_control_types.h"
#include "ethernet_tx_client.h"
#include "misc_timer.h"

typedef struct ethernet_hdr_t {
  unsigned char dest_addr[6];
  unsigned char src_addr[6];
  unsigned char ethertype[2];
} ethernet_hdr_t;

typedef struct tagged_ethernet_hdr_t {
  unsigned char dest_addr[6];
  unsigned char src_addr[6];
  unsigned char qtag[2];
  unsigned char ethertype[2];
} tagged_ethernet_hdr_t;


typedef enum {
  MAAP_DISABLED,
  MAAP_PROBING,
  MAAP_RESERVED
} maap_state_t;

typedef struct {
  unsigned char base[6];
  int  range;
  int  count;
  int  timeout;
  int immediately;
  maap_state_t state;
} maap_address_range;

static unsigned char maap_allocated_lo[6] = MAAP_ALLOCATED_ADDRESS_RANGE_LOW;
static unsigned char maap_allocated_hi[6] = MAAP_ALLOCATED_ADDRESS_RANGE_HIGH;
static unsigned char my_mac_addr[6];
static unsigned char maap_dest_addr[6] = MAAP_PROTOCOL_ADDRESS;
static unsigned long long allocated_range_size = 0;

static unsigned int r=0;
static unsigned int a=1664525;
static unsigned int c=1013904223;

#define RAND(x) do {x = a*x+c;} while (0)

static maap_address_range maap_addr;

static unsigned int maap_buf[(MAX_AVB_1722_MAAP_PDU_SIZE+1)/4];

static unsigned long long mac_addr_to_num(unsigned char addr[6]) 
{
  unsigned long long x = 0;
  for (int i=0;i<6;i++) {
    x += (((unsigned long long) addr[5-i]) << (i*8));
  }
  return x;
}

static void num_to_mac_addr(unsigned char addr[6], 
                            unsigned long long x)
{
  for (int i=0;i<6;i++)  {
    addr[5-i] = x & 0xff;
    x >>= 8;
  }
  return;
}

static int create_maap_packet(int message_type,
                              maap_address_range *addr,
                              char *buf,
                              unsigned char *conflict_addr,
                              int conflict_count)
{ 
  struct ethernet_hdr_t *hdr = (ethernet_hdr_t*) &buf[0];
  struct maap_packet_t *pkt = (maap_packet_t*) (hdr + 1);

  for (int i=0;i<6;i++) {
    hdr->src_addr[i] = my_mac_addr[i];
    hdr->dest_addr[i] = maap_dest_addr[i];
  }
  
  hdr->ethertype[0] = AVB_ETYPE >> 8;
  hdr->ethertype[1] = AVB_ETYPE & 0xff;
  

  SET_MAAP_CD_FLAG(pkt, DEFAULT_MAAP_CD_FLAG);
  SET_MAAP_SUBTYPE(pkt, DEFAULT_MAAP_SUBTYPE);
  SET_MAAP_AVB_VERSION(pkt, DEFAULT_MAAP_AVB_VERSION);
  SET_MAAP_VERSION(pkt, DEFAULT_MAAP_VERSION);
  SET_MAAP_DATALENGTH(pkt, sizeof(maap_packet_t));
  
  SET_MAAP_MSG_TYPE(pkt, message_type);
  for (int i=0;i<6;i++)
    pkt->request_start_address[i] = addr->base[i];

  SET_MAAP_REQUESTED_COUNT(pkt, addr->range);

  if (message_type == MAAP_DEFEND) {
    for (int i=0;i<6;i++)
      pkt->conflict_start_address[i] = conflict_addr[i];
    
    SET_MAAP_CONFLICT_COUNT(pkt, conflict_count);
    
  }
  else {
    for (int i=0;i<6;i++)
      pkt->conflict_start_address[i] = 0;
    
    SET_MAAP_CONFLICT_COUNT(pkt, 0);
  }
  return (64);
}

void avb_1722_maap_init(unsigned char macaddr[6]) 
{
  for (int i=0;i<6;i++) 
    my_mac_addr[i] = macaddr[i];

  maap_addr.state = MAAP_DISABLED;
  // set the random seed 
  r = macaddr[0] + 
     (macaddr[1] << 5)  +
     (macaddr[2] << 10)  +
     (macaddr[3] << 15)  +
     (macaddr[4] << 20)  +
     (macaddr[5] << 25)  +
     (macaddr[6]);

  for (int i=0;i<10;i++)
    RAND(r);

  allocated_range_size = 
    mac_addr_to_num(maap_allocated_hi) - mac_addr_to_num(maap_allocated_lo);
}

void avb_1722_maap_request_addresses(int num_addr, char *start_address) 
{
  // set address range  
  if (start_address) {
    for (int i=0;i<6;i++) {
      maap_addr.base[i] = start_address[i];    
    }
  }
  else {
    unsigned long long offset;

    // Set the base address randomly in the allocated maap address range
    RAND(r);
    offset = r;
    offset <<= 32;
    RAND(r);
    offset += r;
    offset = offset % (allocated_range_size - num_addr);
    
    num_to_mac_addr(maap_addr.base, 
                    mac_addr_to_num(maap_allocated_lo) + offset);
                       
  }
  maap_addr.range = num_addr;
  maap_addr.state = MAAP_PROBING;
  maap_addr.count = MAAP_PROBE_RETRANSMITS;
  maap_addr.immediately = 1;
}


void avb_1722_maap_rerequest_addresses() 
{
  if (maap_addr.state != MAAP_DISABLED) {
    maap_addr.state = MAAP_PROBING;
    maap_addr.count = MAAP_PROBE_RETRANSMITS;
    maap_addr.immediately = 1;
  }
}

void avb_1722_maap_relinquish_addresses()
{
	maap_addr.state = MAAP_DISABLED;
}

void avb_1722_maap_get_offset_address(unsigned char addr[6], int offset)
{  
  long long base = mac_addr_to_num(maap_addr.base);

  num_to_mac_addr(addr, base + offset);

}

void avb_1722_maap_get_base_address(unsigned char addr[6])
{
  avb_1722_maap_get_offset_address(addr, 0);
}

static void set_timeout(maap_address_range *addr,
                        unsigned current_time,
                        unsigned base_ms,
                        unsigned variation_ms)
{
  RAND(r);
  addr->timeout = current_time + base_ms + (r % variation_ms);
  return;
}

int current_time_to_milliseconds(int current_time)
{
  static int last_time=0;
  static int first=1;
  static int time_ms=0;
  static int time_lo=0;
  if (first) {
    time_ms = current_time / 100000;
    first = 0;
  }
  else {
    unsigned int diff = (current_time - last_time);
    time_ms += diff / 100000;
    time_lo += diff % 100000;
    if (time_lo > 100000) {
      time_lo -= 100000;
      time_ms += 1;
    }
    last_time = current_time;
  }
  
  return time_ms;
}

int avb_1722_maap_periodic(chanend c_tx)
{
  int status;
  int nbytes;
  int current_time = current_time_to_milliseconds(get_local_time());
  status = AVB_NO_STATUS;
  switch (maap_addr.state) 
    {
    case MAAP_DISABLED:
      break;
    case MAAP_PROBING:
      if (maap_addr.immediately || (current_time - maap_addr.timeout) > 0) {
        maap_addr.immediately = 0;
        nbytes = create_maap_packet(MAAP_PROBE, 
                                    &maap_addr, 
                                    (char *) &maap_buf[0],
                                    NULL,
                                    0);
        
        mac_tx(c_tx, maap_buf, nbytes, ETH_BROADCAST);
        maap_addr.count--;

        if (maap_addr.count == 0) {
          maap_addr.state = MAAP_RESERVED;
          status = AVB_MAAP_ADDRESSES_RESERVED;
          maap_addr.immediately = 1;
        }
        else {
          // reset timeout
          set_timeout(&maap_addr,
                      current_time,
                      MAAP_PROBE_INTERVAL_BASE_MS,
                      MAAP_PROBE_INTERVAL_VARIATION_MS);
        }
      }
      break;
    case MAAP_RESERVED:
      if (maap_addr.immediately || (current_time - maap_addr.timeout) > 0) {
        maap_addr.immediately = 0;
        nbytes = create_maap_packet(MAAP_ANNOUNCE, 
                                    &maap_addr, 
                                    (char *) &maap_buf[0],
                                    NULL,
                                    0);
        mac_tx(c_tx, maap_buf, nbytes, ETH_BROADCAST);
        // set timeout
        set_timeout(&maap_addr, 
                    current_time,
                    MAAP_ANNOUNCE_INTERVAL_BASE_MS,
                    MAAP_ANNOUNCE_INTERVAL_VARIATION_MS);                     
      }
      break;
    } 
  return status;
}

static int maap_conflict(maap_address_range* addr,
                         maap_packet_t* pkt,
                         unsigned char conflict_addr[6],
                         int *conflict_count)
{
  unsigned long long my_addr;
  unsigned long long conflict_lo;
  unsigned long long conflict_hi;
  int count = 0;

  my_addr = mac_addr_to_num(addr->base);
  conflict_lo = mac_addr_to_num(pkt->conflict_start_address);
  conflict_hi = conflict_lo + GET_MAAP_CONFLICT_COUNT(pkt);

  // iterate over address range to see if any conflict
  for (int j = 0; j < addr->range; my_addr++, j++) {
    if (my_addr >= conflict_lo && my_addr <= conflict_hi) {
        // We have a conflict
       if (count == 0)
    	   num_to_mac_addr(conflict_addr, my_addr);
       count++;
    }
  }
  *conflict_count = count;
  return (count > 0);
}

avb_status_t avb_1722_maap_process_packet_(unsigned int buf0[], 
                                 int nbytes,
                                 chanend c_tx)
{
  unsigned char *buf = (unsigned char *) buf0;
  int msg_type;
  struct ethernet_hdr_t *ethernet_hdr = (ethernet_hdr_t *) &buf[0];
  struct tagged_ethernet_hdr_t *tagged_ethernet_hdr = 
    (tagged_ethernet_hdr_t *) &buf[0];

  int has_qtag = ethernet_hdr->ethertype[1]==0x18;
  int ethernet_pkt_size = has_qtag ? 18 : 14;
  unsigned char conflict_addr[6];
  int conflict_count;
  struct maap_packet_t *maap_pkt = 
    (struct maap_packet_t *) &buf[ethernet_pkt_size];

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


  if (GET_MAAP_CD_FLAG(maap_pkt) != 1 ||
      GET_MAAP_SUBTYPE(maap_pkt) != 0x7e)
    // not a maap packet
    return AVB_NO_STATUS;

  if (maap_addr.state == MAAP_DISABLED)
    return AVB_NO_STATUS;

  msg_type = GET_MAAP_MSG_TYPE(maap_pkt);

  switch (msg_type)
    {
    case MAAP_PROBE:
      if (maap_conflict(&maap_addr, maap_pkt, conflict_addr, &conflict_count)) {
        int len;
        len = create_maap_packet(MAAP_DEFEND, &maap_addr, (char*) &maap_buf[0],
                                 conflict_addr, conflict_count);
        mac_tx(c_tx, maap_buf, len, ETH_BROADCAST);
      }
      break;
    case MAAP_ANNOUNCE:
      if (maap_conflict(&maap_addr, maap_pkt, NULL, NULL)) {
        maap_addr.state = MAAP_DISABLED;
        return AVB_MAAP_ADDRESSES_LOST;
      }
      break;
    }
  return AVB_NO_STATUS;
}

