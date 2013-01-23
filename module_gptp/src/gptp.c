/* This module implements the 802.1as gptp timing protocol.
   It is a restricted version of the protocol that can only handle
   endpoints with one port. As such it is optimized (particularly for 
   memory usage) and combined the code for the port state machines and the site 
   state machines into one. */
#include <string.h>
#include <limits.h>
#include "avb_conf.h"
#include "gptp.h"
#include "gptp_config.h"
#include "gptp_pdu.h"
#include "ethernet_tx_client.h"
#include "ethernet_rx_client.h"
#include "misc_timer.h"
#include "print.h"
#include "simple_printf.h"
#include "avb_util.h"

//#define GPTP_DEBUG 1

#define timeafter(A, B) ((int)((B) - (A)) < 0)

// Config variables
static int ptp_master_only_pdelay_resp = 0;
static int ptp_legacy_mode = 0;

#define NANOSECONDS_PER_SECOND (1000000000)

/* The adjust between local clock ticks and ptp clock ticks.
   This is the ratio between our clock speed and the grandmaster less 1. 
   For example, if we are running 1% faster than the master clock then 
   this value will be 0.01 */
#define PTP_ADJUST_WEIGHT 32
static int g_ptp_adjust_valid = 0;
signed g_ptp_adjust = 0;
signed g_inv_ptp_adjust = 0;

/* The average path delay (over the last PDELAY_AVG_WINDOW pdelay_reqs) 
   between the foreign master port and our slave port in nanoseconds (ptp time)
*/
#define PTP_PATH_DELAY_WEIGHT 32
static int ptp_path_delay_valid = 0;
unsigned ptp_path_delay = 0;

/* These variables make up the state of the local clock/port */
unsigned ptp_reference_local_ts;
ptp_timestamp ptp_reference_ptp_ts;
static long long ptp_gmoffset = 0;
static int expect_gm_discontinuity = 1;
static int ptp_candidate_gmoffset_valid = 0;
static ptp_state_t ptp_state = PTP_MASTER;
static n80_t my_port_id;
static n80_t master_port_id;
static u8_t ptp_priority1;
static u8_t ptp_priority2 = PTP_DEFAULT_PRIORITY2;

/* Timing variables */
static unsigned last_received_announce_time_valid=0;
static unsigned last_received_announce_time;
static unsigned last_announce_time;
static unsigned last_sync_time;
static unsigned last_pdelay_req_time;

static ptp_timestamp prev_adjust_master_ts;
static unsigned prev_adjust_local_ts;
static int prev_adjust_valid = 0;

static int sync_lock = 0;
static int sync_count = 0;

static AnnounceMessage best_announce_msg;

static unsigned long long pdelay_epoch_timer;
static unsigned prev_pdelay_local_ts;

static int tile_timer_offset;


ptp_state_t ptp_current_state()
{
  return ptp_state;
}

unsigned local_timestamp_to_ptp_mod32(unsigned local_ts,
                                      ptp_time_info_mod64 *info)
{
  long long local_diff = (signed) local_ts - (signed) info->local_ts;

  local_diff *= 10;
  local_diff = local_diff + ((local_diff * info->ptp_adjust) >> PTP_ADJUST_PREC);  

  return (info->ptp_ts_lo + (int) local_diff);
}

void local_timestamp_to_ptp_mod64(unsigned local_ts,
                                  ptp_time_info_mod64 *info,
                                  unsigned *hi,
                                  unsigned *lo)
{
  long long local_diff = (signed) local_ts - (signed) info->local_ts;
  unsigned long long ptp_mod64 = ((unsigned long long) info->ptp_ts_hi << 32) + info->ptp_ts_lo;

  local_diff *= 10;
  local_diff = local_diff + ((local_diff * info->ptp_adjust) >> PTP_ADJUST_PREC);  

  ptp_mod64 += local_diff;

  *hi = ptp_mod64 >> 32;
  *lo = (unsigned) ptp_mod64;
  return;
}



void ptp_get_reference_ptp_ts_mod_64(unsigned *hi, unsigned *lo)
{
  unsigned long long t;
  t = ptp_reference_ptp_ts.seconds[0] +  ((unsigned long long) ptp_reference_ptp_ts.seconds[1] << 32);
  t = t * NANOSECONDS_PER_SECOND;
  t += ptp_reference_ptp_ts.nanoseconds;
  *hi = (unsigned) (t >> 32);
  *lo = (unsigned) t;
}

static long long local_time_to_ptp_time(unsigned t, int l_ptp_adjust)
{
  long long ret = ((long long) t)*10;

  if (g_ptp_adjust_valid) {
    ret = ret + ((ret * l_ptp_adjust) >> PTP_ADJUST_PREC);
  }
  return ret;
}


static void ptp_timestamp_offset64(ptp_timestamp *dst,
                                   ptp_timestamp *ts,
                                   long long offset)
{
  unsigned long long sec = ts->seconds[0] |  
                           ((unsigned long long) ts->seconds[1] << 32);

  unsigned long long nanosec = ts->nanoseconds;

  nanosec = nanosec + offset;
  
  sec = sec + nanosec / NANOSECONDS_PER_SECOND;
  
  nanosec = nanosec % NANOSECONDS_PER_SECOND;

  dst->seconds[1] = (unsigned) (sec >> 32);

  dst->seconds[0] = (unsigned) sec;
  
  dst->nanoseconds = nanosec;

  return;
}


void ptp_timestamp_offset(ptp_timestamp *ts, int offset)
{
  ptp_timestamp_offset64(ts, ts, offset);
}

static long long ptp_timestamp_diff(ptp_timestamp *a,
                                    ptp_timestamp *b)
{
  unsigned long long sec_a = a->seconds[0] |  
                           ((unsigned long long) a->seconds[1] << 32);
  unsigned long long sec_b = b->seconds[0] |  
                           ((unsigned long long) b->seconds[1] << 32);
  unsigned long long nanosec_a = a->nanoseconds;
  unsigned long long nanosec_b = b->nanoseconds;

  long long sec_diff = sec_a - sec_b;
  long long nanosec_diff = nanosec_a - nanosec_b;

  nanosec_diff += sec_diff * NANOSECONDS_PER_SECOND;
  
  return nanosec_diff;  
}

unsigned ptp_timestamp_to_local(ptp_timestamp *ts,
                                ptp_time_info *info)
{
	  long long ptp_diff;
	  long long local_diff;
	  ptp_diff = ptp_timestamp_diff(ts, &info->ptp_ts);

	  local_diff = ptp_diff + ((ptp_diff * info->inv_ptp_adjust) >> PTP_ADJUST_PREC);
	  local_diff = local_diff / 10;
	  return (info->local_ts + local_diff);
}

unsigned ptp_mod32_timestamp_to_local(unsigned ts, ptp_time_info_mod64* info)
{
	  long long ptp_diff;
	  long long local_diff;
	  ptp_diff = (signed) ts - (signed)info->ptp_ts_lo;

	  local_diff = ptp_diff + ((ptp_diff * info->inv_ptp_adjust) >> PTP_ADJUST_PREC);
	  local_diff = local_diff / 10;
	  return (info->local_ts + local_diff);
}

static void _local_timestamp_to_ptp(ptp_timestamp *ptp_ts,
                                    unsigned local_ts,
                                    unsigned reference_local_ts,
                                    ptp_timestamp *reference_ptp_ts,
                                    unsigned ptp_adjust)
{
  unsigned local_diff = (signed) local_ts - (signed) reference_local_ts;
  
  unsigned long long diff = local_time_to_ptp_time(local_diff, ptp_adjust);

  ptp_timestamp_offset64(ptp_ts, reference_ptp_ts, diff);

  return;
}                

void local_timestamp_to_ptp(ptp_timestamp *ptp_ts,
                            unsigned local_ts,
                            ptp_time_info *info)
{
  _local_timestamp_to_ptp(ptp_ts,
                          local_ts, 
                          info->local_ts,
                          &info->ptp_ts,
                          info->ptp_adjust);

  return;
}

#define local_to_ptp_ts(ptp_ts, local_ts) _local_timestamp_to_ptp(ptp_ts, local_ts, ptp_reference_local_ts, &ptp_reference_ptp_ts, g_ptp_adjust)

static void create_my_announce_msg(AnnounceMessage *pAnnounceMesg);

static void set_new_role(enum ptp_state_t new_role,
                         unsigned t) {


  if (new_role == PTP_SLAVE) {

	simple_printf("PTP Role: Slave\n");

    // Reset synotization variables
    ptp_path_delay_valid = 0;
    g_ptp_adjust = 0;
    g_inv_ptp_adjust = 0;
    prev_adjust_valid = 0;
    // Since there has been a role change there may be a gm discontinuity 
    // to detect
    expect_gm_discontinuity = 1;
    ptp_candidate_gmoffset_valid = 0;
    last_pdelay_req_time = t;
    sync_lock = 0;
    sync_count = 0;
  }
  
  if (new_role == PTP_MASTER) {

	simple_printf("PTP Role: Master\n");

    // Now we are the master so no rate matching is needed
    g_ptp_adjust = 0;
    g_inv_ptp_adjust = 0;

    ptp_reference_local_ts = 
      ptp_reference_local_ts;

    ptp_gmoffset = 0;    
    last_sync_time = last_announce_time = t;
  }


  ptp_state = new_role;

  if (ptp_state == PTP_MASTER || ptp_state == PTP_UNCERTAIN)
    create_my_announce_msg(&best_announce_msg);

  return;
}


/* Assume very conservatively that the worst case is that 
   the sync messages a .5sec apart. That is 5*10^9ns which can
   be stored in 29 bits. So we have 35 fractional bits to calculate 
   with */
#define ADJUST_CALC_PREC 35

#define DEBUG_ADJUST

static void update_adjust(ptp_timestamp *master_ts,
                          unsigned local_ts) 
{

  if (prev_adjust_valid) {
    signed long long adjust, inv_adjust, master_diff, local_diff;


    /* Calculated the difference between two sync message on
       the master port and the local port */
    master_diff = ptp_timestamp_diff(master_ts, &prev_adjust_master_ts);
    local_diff = (signed) local_ts - (signed) prev_adjust_local_ts;

    /* The local timestamps are based on 100Mhz. So 
       convert to nanoseconds */
    local_diff *= 10;

    /* Work at the new adjust value in 64 bits */
    adjust = master_diff - local_diff;
    inv_adjust = local_diff - master_diff;

    adjust <<= ADJUST_CALC_PREC;
    inv_adjust <<= ADJUST_CALC_PREC;
    
    if (master_diff == 0 || local_diff == 0) {
      prev_adjust_valid = 0;
      return;
    }

    adjust = adjust / master_diff;
    inv_adjust = inv_adjust / local_diff;

    /* Reduce it down to PTP_ADJUST_PREC */
    adjust >>= (ADJUST_CALC_PREC - PTP_ADJUST_PREC);
    inv_adjust >>= (ADJUST_CALC_PREC - PTP_ADJUST_PREC);
    
    if (adjust >> 32) {
    // Overflow on adjust!!
      
    }    

    /* Re-average the adjust with a given weighting.
       This method loses a few bits of precision */
    if (g_ptp_adjust_valid) {

      long long diff = adjust - (long long) g_ptp_adjust;
      
      if (diff < 0) 
        diff = -diff;

      if (!sync_lock) {
        if (diff < PTP_SYNC_LOCK_ACCEPTABLE_VARIATION) {
          sync_count++;
          if (sync_count > PTP_SYNC_LOCK_STABILITY_COUNT) {
            printstr("PTP sync locked\n");
            sync_lock = 1;
            sync_count = 0;
          }
        }
        else 
          sync_count = 0;
      }
      else {
        if (diff > PTP_SYNC_LOCK_ACCEPTABLE_VARIATION) {
          sync_count++;
          if (sync_count > PTP_SYNC_LOCK_STABILITY_COUNT) {
            printstr("PTP sync lock lost\n");
            sync_lock = 0;
            sync_count = 0;
          }
        }
        else 
          sync_count = 0;
      }

      adjust = (((long long)g_ptp_adjust) * (PTP_ADJUST_WEIGHT - 1) + adjust) / PTP_ADJUST_WEIGHT;

      g_ptp_adjust = (int) adjust;

      inv_adjust = (((long long)g_inv_ptp_adjust) * (PTP_ADJUST_WEIGHT - 1) + inv_adjust) / PTP_ADJUST_WEIGHT;

      g_inv_ptp_adjust = (int) inv_adjust;
    }
    else {
      g_ptp_adjust = (int) adjust;
      g_inv_ptp_adjust = (int) inv_adjust;
      g_ptp_adjust_valid = 1;
    }
  }

  prev_adjust_local_ts = local_ts;
  prev_adjust_master_ts = *master_ts;
  prev_adjust_valid = 1;
  return;
}

static void update_reference_timestamps(ptp_timestamp *master_egress_ts,
                                        unsigned local_ingress_ts)
{
  ptp_timestamp master_ingress_ts;

  ptp_timestamp_offset64(&master_ingress_ts, master_egress_ts, ptp_path_delay);

  /* Update the reference timestamps */
  ptp_reference_local_ts = local_ingress_ts;
  ptp_reference_ptp_ts = master_ingress_ts;
  
  return;
}

#define UPDATE_REFERENCE_TIMESTAMP_PERIOD (500000000) // 5 sec

static void periodic_update_reference_timestamps(unsigned int local_ts)
{
  
  int local_diff = local_ts - ptp_reference_local_ts;

  

  if (local_diff > UPDATE_REFERENCE_TIMESTAMP_PERIOD) {
    long long ptp_diff = local_time_to_ptp_time(local_diff, g_ptp_adjust);

    ptp_reference_local_ts = local_ts;
    ptp_timestamp_offset64(&ptp_reference_ptp_ts, 
                           &ptp_reference_ptp_ts,
                           ptp_diff);
  }
  return;
}


static void update_path_delay(ptp_timestamp *master_ingress_ts,
                              ptp_timestamp *master_egress_ts,
                              unsigned local_egress_ts,
                              unsigned local_ingress_ts)
{
  long long master_diff;
  long long local_diff;
  long long delay;
  long long round_trip;

  /* The sequence of events is:
     
     local egress   (ptp req sent from our local port)
     master ingress (ptp req recv on master port) 
     master egress  (ptp resp sent from master port)
     local ingress  (ptp resp recv on our local port)
   
     So transit time (assuming a symetrical link) is:

     ((local_ingress_ts - local_egress_ts) - (master_egress_ts - master_ingress_ts) ) / 2

  */
  
  master_diff = ptp_timestamp_diff(master_egress_ts,  master_ingress_ts);

  local_diff = (signed) local_ingress_ts - (signed) local_egress_ts;
  
  local_diff = local_time_to_ptp_time(local_diff, g_ptp_adjust);

  round_trip = (local_diff - master_diff);

  round_trip -= LOCAL_EGRESS_DELAY;

  delay = round_trip / 2;

  if (delay < 0)
    delay = 0;

  if (ptp_path_delay_valid) {

    // TODO - Sanity check

    /* Re-average the adjust with a given weighting.
       This method loses a few bits of precision */
    ptp_path_delay = ((ptp_path_delay * (PTP_PATH_DELAY_WEIGHT - 1)) + (int) delay) / PTP_PATH_DELAY_WEIGHT;
    //ptp_path_delay = delay;
    }
  else {
    ptp_path_delay = delay;
    ptp_path_delay_valid = 1;
  }

  return;
}

/* Returns:
      -1 - if clock is worse than me
      1  - if clock is better than me
      0  - if clocks are equal
*/ 
static int compare_clock_identity_to_me(n64_t *clockIdentity)
{
  for (int i=0;i<8;i++)
    if (clockIdentity->data[i] > my_port_id.data[i])
      return -1;
    else if (clockIdentity->data[i] < my_port_id.data[i])
      return 1;
  
  // Thje two clock identities are the same
  return 0;
}

static int compare_clock_identity(n64_t *c1,
                                  n64_t *c2)
{
  for (int i=0;i<8;i++)
    if (c1->data[i] > c2->data[i])
      return -1;
    else if (c1->data[i] < c2->data[i])
      return 1;
  
  // Thje two clock identities are the same
  return 0;
}

static void bmca_update_roles(char *msg, unsigned t)
{
  ComMessageHdr *pComMesgHdr = (ComMessageHdr *) msg;
  AnnounceMessage *pAnnounceMesg = (AnnounceMessage *) ((char *) pComMesgHdr+sizeof(ComMessageHdr));
  int clock_identity_comp;
  int new_best = 0;

  clock_identity_comp =
    compare_clock_identity_to_me(&pAnnounceMesg->grandmasterIdentity);

  if (clock_identity_comp == 0) {
    /* If the message is about me then we win since our stepsRemoved is 0 */
  }
  else {
   /* Message is from a different clock. Let's work out if it is better or 
      worse according to the 802.1as BCMA */ 
    if (pAnnounceMesg->grandmasterPriority1 > best_announce_msg.grandmasterPriority1) {
    }
    else if (pAnnounceMesg->grandmasterPriority1 < best_announce_msg.grandmasterPriority1) {
      new_best = 1;
    }
    else if (pAnnounceMesg->clockClass > best_announce_msg.clockClass)  {
    }
    else if (pAnnounceMesg->clockClass < best_announce_msg.clockClass) {
     new_best = 1;
    }
    else if (pAnnounceMesg->clockAccuracy > best_announce_msg.clockAccuracy) {
    }
    else if (pAnnounceMesg->clockAccuracy < best_announce_msg.clockAccuracy) {
     new_best = 1;
    }
    else if (ntoh16(pAnnounceMesg->clockOffsetScaledLogVariance) > ntoh16(best_announce_msg.clockOffsetScaledLogVariance)) {
    }
    else if (ntoh16(pAnnounceMesg->clockOffsetScaledLogVariance) < ntoh16(best_announce_msg.clockOffsetScaledLogVariance)) {
     new_best = 1;
    }
    else if (pAnnounceMesg->grandmasterPriority2 > best_announce_msg.grandmasterPriority2) {
    }
    else if (pAnnounceMesg->grandmasterPriority2 < best_announce_msg.grandmasterPriority2) {
     new_best = 1;   
    }
    else
      {
        clock_identity_comp = 
          compare_clock_identity(&pAnnounceMesg->grandmasterIdentity,
                                 &best_announce_msg.grandmasterIdentity);
                                                      
        if (clock_identity_comp <= 0) {
          // 
        }
        else  {
          new_best = 1;
        }
      }
  }


  if (new_best) {
    memcpy(&best_announce_msg, pAnnounceMesg, sizeof(AnnounceMessage));
    master_port_id = pComMesgHdr->sourcePortIdentity;          

    {
      set_new_role(PTP_SLAVE, t);
      last_received_announce_time_valid = 0;    
      master_port_id = pComMesgHdr->sourcePortIdentity;    
    }
  }

  return;
}


static void timestamp_to_network(n80_t *msg,
                                 ptp_timestamp *ts)
{
  char *sec0_p = (char *) &ts->seconds[0];
  char *sec1_p = (char *) &ts->seconds[1];
  char *nsec_p = (char *) &ts->nanoseconds;

  // Convert seconds to big-endian
  msg->data[0] = sec1_p[3];
  msg->data[1] = sec1_p[2];

  msg->data[2] = sec0_p[3];
  msg->data[3] = sec0_p[2];
  msg->data[4] = sec0_p[1];
  msg->data[5] = sec0_p[0];
                       
  // Now convert nanoseconds
  msg->data[6] = nsec_p[3];
  msg->data[7] = nsec_p[2];
  msg->data[8] = nsec_p[1];
  msg->data[9] = nsec_p[0];
  
  return;
}

static void network_to_ptp_timestamp(ptp_timestamp *ts,
                                     n80_t *msg)
{
  char *sec0_p = (char *) &ts->seconds[0];
  char *sec1_p = (char *) &ts->seconds[1];
  char *nsec_p = (char *) &ts->nanoseconds;

  sec1_p[3] = msg->data[0];
  sec1_p[2] = msg->data[1];
  sec1_p[1] = 0;
  sec1_p[0] = 0;
                             
  sec0_p[3] = msg->data[2];
  sec0_p[2] = msg->data[3];
  sec0_p[1] = msg->data[4];
  sec0_p[0] = msg->data[5];
                           
  nsec_p[3] = msg->data[6];
  nsec_p[2] = msg->data[7];
  nsec_p[1] = msg->data[8];
  nsec_p[0] = msg->data[9];
  
  return;
}

static int port_id_equal(n80_t *a, n80_t *b)
{
  for (int i=0;i<10;i++)
    if (a->data[i] != b->data[i])
      return 0;
  return 1;
}

static int clock_id_equal(n64_t *a, n64_t *b)
{
  for (int i=0;i<8;i++)
    if (a->data[i] != b->data[i])
      return 0;
  return 1;
}


static void ptp_tx(chanend c_tx,
                   unsigned int *buf,
                   int len)
{
  len = len < 64 ? 64 : len;
  mac_tx(c_tx, buf, len, 0);
  return;
}

static void ptp_tx_timed(chanend c_tx,
                         unsigned int *buf,
                         int len,
                         unsigned *ts)
{
  len = len < 64 ? 64 : len;
  mac_tx_timed(c_tx, buf, len, ts, 0);
  *ts = *ts - tile_timer_offset;
  return;
}

static unsigned char src_mac_addr[6];
static unsigned char dest_mac_addr[6] = PTP_DEFAULT_DEST_ADDR;
static unsigned char non_legacy_dest_mac_addr[6] = PTP_8021AS_DEST_ADDR;
static unsigned char legacy_dest_mac_addr[6] = PTP_8021AS_LEGACY_ADDR;


static void set_ptp_ethernet_hdr(unsigned char *buf)
{
  ethernet_hdr_t *hdr = (ethernet_hdr_t *) buf;

  for (int i=0;i<6;i++)  {
    hdr->src_addr[i] = src_mac_addr[i];
    hdr->dest_addr[i] = dest_mac_addr[i];
  }
  
  hdr->ethertype[0] = (PTP_ETHERTYPE >> 8);
  hdr->ethertype[1] = (PTP_ETHERTYPE & 0xff);

  return;
}

// Estimate of announce message processing time delay.
#define MESSAGE_PROCESS_TIME    (3563)

static u16_t announce_seq_id = 0;

static void create_my_announce_msg(AnnounceMessage *pAnnounceMesg) 
{
   // setup the Announce message
   pAnnounceMesg->currentUtcOffset = hton16(PTP_CURRENT_UTC_OFFSET);
   pAnnounceMesg->grandmasterPriority1 = ptp_priority1;

   // grandMaster clock quality.
   pAnnounceMesg->clockClass = PTP_CLOCK_CLASS;
   pAnnounceMesg->clockAccuracy = PTP_CLOCK_ACCURACY;

   pAnnounceMesg->clockOffsetScaledLogVariance = 
     hton16(PTP_OFFSET_SCALED_LOG_VARIANCE);
   
   // grandMaster priority
   pAnnounceMesg->grandmasterPriority2 = ptp_priority2;

   for (int i=0;i<8;i++)
     pAnnounceMesg->grandmasterIdentity.data[i] = my_port_id.data[i];
   
   pAnnounceMesg->stepsRemoved = hton16(0);

   pAnnounceMesg->timeSource = PTP_TIMESOURCE;

   pAnnounceMesg->tlvType = hton16(PTP_ANNOUNCE_TLV_TYPE);
   pAnnounceMesg->tlvLength = hton16(8);
   
   for (int i=0;i<7;i++) 
     pAnnounceMesg->pathSequence.data[i] = my_port_id.data[i];
}

static void send_ptp_announce_msg(chanend c_tx)
{
#define ANNOUNCE_PACKET_SIZE (sizeof(ethernet_hdr_t) + sizeof(ComMessageHdr) + sizeof(AnnounceMessage))
  unsigned int buf0[(ANNOUNCE_PACKET_SIZE+3)/4];
  unsigned char *buf = (unsigned char *) &buf0[0];
  ComMessageHdr *pComMesgHdr = (ComMessageHdr *) &buf[sizeof(ethernet_hdr_t)];
  AnnounceMessage *pAnnounceMesg = (AnnounceMessage *) &buf[sizeof(ethernet_hdr_t) + sizeof(ComMessageHdr)];
  unsigned cur_local_time;
  ptp_timestamp cur_time_ptp;

  set_ptp_ethernet_hdr(buf);

  // setup the common message header.
  memset(pComMesgHdr, 0, sizeof(ComMessageHdr) + sizeof(AnnounceMessage));

  pComMesgHdr->transportSpecific_messageType = 
    PTP_TRANSPORT_SPECIFIC_HDR | PTP_ANNOUNCE_MESG;
  
   pComMesgHdr->versionPTP = PTP_VERSION_NUMBER;
   pComMesgHdr->messageLength = hton16(sizeof(ComMessageHdr) + 
                                       sizeof(AnnounceMessage));
   pComMesgHdr->flagField[1] = 
     ((PTP_LEAP61 & 0x1)) |
     ((PTP_LEAP59 & 0x1) << 1) | 
     ((PTP_CURRENT_UTC_OFFSET_VALID & 0x1) << 2) |
     ((PTP_TIMESCALE & 0x1) << 3) |
     ((PTP_TIME_TRACEABLE & 0x1) << 4) |
     ((PTP_FREQUENCY_TRACEABLE & 0x1) << 5);

   pComMesgHdr->flagField[0]                 = 0x2;   // set two steps flag

   // portId assignment
   pComMesgHdr->sourcePortIdentity = my_port_id;

   // sequence id.
   announce_seq_id += 1;
   pComMesgHdr->sequenceId = hton16(announce_seq_id);

   pComMesgHdr->controlField = PTP_CTL_FIELD_OTHERS;

   pComMesgHdr->logMessageInterval = PTP_LOG_ANNOUNCE_INTERVAL;


   create_my_announce_msg(pAnnounceMesg);
   
   // time stamp originTS
   cur_local_time = get_local_time() + MESSAGE_PROCESS_TIME;

   local_to_ptp_ts(&cur_time_ptp, cur_local_time);

   timestamp_to_network(&pAnnounceMesg->originTimestamp, &cur_time_ptp);

   // send the message.
   ptp_tx(c_tx, buf0, ANNOUNCE_PACKET_SIZE);

   return;
}


static u16_t sync_seq_id = 0;

static void send_ptp_sync_msg(chanend c_tx)
{
 #define SYNC_PACKET_SIZE (sizeof(ethernet_hdr_t) + sizeof(ComMessageHdr) + sizeof(SyncMessage))
 #define FOLLOWUP_PACKET_SIZE (sizeof(ethernet_hdr_t) + sizeof(ComMessageHdr) + sizeof(FollowUpMessage))
  unsigned int buf0[(FOLLOWUP_PACKET_SIZE+3)/4]; 
  unsigned char *buf = (unsigned char *) &buf0[0];
  ComMessageHdr *pComMesgHdr = (ComMessageHdr *) &buf[sizeof(ethernet_hdr_t)];;
  SyncMessage *pSyncMesg = (SyncMessage *) &buf[sizeof(ethernet_hdr_t) +
                                                sizeof(ComMessageHdr)];
  FollowUpMessage *pFollowUpMesg = (FollowUpMessage *) &buf[sizeof(ethernet_hdr_t) + sizeof(ComMessageHdr)];

  unsigned cur_local_time;
  ptp_timestamp cur_time_ptp;
  unsigned local_egress_ts;
  ptp_timestamp ptp_egress_ts;

  set_ptp_ethernet_hdr(buf);

  memset(pComMesgHdr, 0, sizeof(ComMessageHdr) + sizeof(FollowUpMessage));

  // 1. Send Sync message.
  
  pComMesgHdr->transportSpecific_messageType =
    PTP_TRANSPORT_SPECIFIC_HDR | PTP_SYNC_MESG;
  
  pComMesgHdr->versionPTP = PTP_VERSION_NUMBER;
  
  pComMesgHdr->messageLength = hton16(sizeof(ComMessageHdr) + 
                                      sizeof(SyncMessage));

  pComMesgHdr->flagField[0]                 = 0x2;   // set two steps flag
  
  for(int i=0;i<8;i++) pComMesgHdr->correctionField.data[i] = 0;

  pComMesgHdr->sourcePortIdentity = my_port_id;
  
  sync_seq_id += 1;
  
  pComMesgHdr->sequenceId = hton16(sync_seq_id);

  pComMesgHdr->controlField = PTP_CTL_FIELD_SYNC;
  
  pComMesgHdr->logMessageInterval = PTP_LOG_SYNC_INTERVAL;
  
  // extract the current time.
  cur_local_time = get_local_time() + MESSAGE_PROCESS_TIME;

  local_to_ptp_ts(&cur_time_ptp, cur_local_time);

  timestamp_to_network(&pSyncMesg->originTimestamp, &cur_time_ptp);
  
  // transmit the packet and record the egress time.
  ptp_tx_timed(c_tx, buf0, 
               SYNC_PACKET_SIZE,
               &local_egress_ts);
    
  // Send Follow_Up message
  
  pComMesgHdr->transportSpecific_messageType = 
    PTP_TRANSPORT_SPECIFIC_HDR | PTP_FOLLOW_UP_MESG;
  
  pComMesgHdr->controlField = PTP_CTL_FIELD_FOLLOW_UP;

  pComMesgHdr->messageLength = hton16(sizeof(ComMessageHdr) + 
                                      sizeof(FollowUpMessage));
  
  // populate the time in packet
  local_to_ptp_ts(&ptp_egress_ts, local_egress_ts);
  
  timestamp_to_network(&pFollowUpMesg->preciseOriginTimestamp, &ptp_egress_ts);

  // Fill in follow up fields as per 802.1as section 11.4.4.2
  pFollowUpMesg->tlvType = hton16(0x3);
  pFollowUpMesg->lengthField = hton16(28);
  pFollowUpMesg->organizationId[0] = 0x00;
  pFollowUpMesg->organizationId[1] = 0x80;
  pFollowUpMesg->organizationId[2] = 0xc2;
  pFollowUpMesg->organizationSubType[0] = 0;
  pFollowUpMesg->organizationSubType[1] = 0;
  pFollowUpMesg->organizationSubType[2] = 1;
  
  
  
  ptp_tx(c_tx, buf0, FOLLOWUP_PACKET_SIZE);

  return;
}

static u16_t pdelay_req_seq_id = 0;
static unsigned pdelay_request_sent = 0;
static unsigned pdelay_request_sent_ts;

static void send_ptp_pdelay_req_msg(chanend c_tx)
{
  unsigned int buf0[(PDELAY_REQ_PACKET_SIZE+3)/4];
  unsigned char *buf = (unsigned char *) &buf0[0];
  ComMessageHdr *pComMesgHdr = (ComMessageHdr *) &buf[sizeof(ethernet_hdr_t)]; 
  PdelayReqMessage *pTxReqHdr = (PdelayReqMessage *) &buf[sizeof(ethernet_hdr_t) + sizeof(pComMesgHdr)];

  unsigned cur_local_time;
  ptp_timestamp cur_time_ptp;

  set_ptp_ethernet_hdr(buf);   

  // clear the send data first.
  memset(pComMesgHdr, 0, sizeof(ComMessageHdr) + sizeof(PdelayReqMessage));

  // build up the packet as required.
  pComMesgHdr->transportSpecific_messageType = 
    PTP_TRANSPORT_SPECIFIC_HDR | PTP_PDELAY_REQ_MESG;

  pComMesgHdr->versionPTP = PTP_VERSION_NUMBER;

  pComMesgHdr->messageLength = hton16(sizeof(ComMessageHdr) + 
                                      sizeof(PdelayReqMessage));

  // correction field, & flagField are zero.
  pComMesgHdr->sourcePortIdentity = my_port_id;
  for(int i=0;i<8;i++) pComMesgHdr->correctionField.data[i] = 0;

  // increment the sequence id.
  pdelay_req_seq_id += 1;
  pComMesgHdr->sequenceId = hton16(pdelay_req_seq_id);

  // control field for backward compatiability
  pComMesgHdr->controlField = PTP_CTL_FIELD_OTHERS;
  pComMesgHdr->logMessageInterval = 0x7F;


  // populate the orginTimestamp.
  cur_local_time = get_local_time() + MESSAGE_PROCESS_TIME;

  local_to_ptp_ts(&cur_time_ptp, cur_local_time);
  
  timestamp_to_network(&pTxReqHdr->originTimestamp, &cur_time_ptp);

  // sent out the data and record the time.

  ptp_tx_timed(c_tx, buf0,
               PDELAY_REQ_PACKET_SIZE,
               &pdelay_request_sent_ts);

  pdelay_request_sent = 1;

  return;
}

void local_to_epoch_ts(unsigned local_ts, ptp_timestamp *epoch_ts)
{
  unsigned long long sec;
  unsigned long long nanosec;

  if (local_ts <= prev_pdelay_local_ts) // We overflowed 32 bits
  {
    pdelay_epoch_timer += ((UINT_MAX - prev_pdelay_local_ts) + local_ts);
  }
  else
  {
    pdelay_epoch_timer += (local_ts - prev_pdelay_local_ts);
  }

  nanosec = pdelay_epoch_timer * 10;

  sec = nanosec / NANOSECONDS_PER_SECOND;
  nanosec = nanosec % NANOSECONDS_PER_SECOND;

  epoch_ts->seconds[1] = (unsigned) (sec >> 32);

  epoch_ts->seconds[0] = (unsigned) sec;
  
  epoch_ts->nanoseconds = nanosec;

  prev_pdelay_local_ts = local_ts;

}

static void send_ptp_pdelay_resp_msg(chanend c_tx, 
                              char *pdelay_req_msg,
                              unsigned req_ingress_ts)
{
#define PDELAY_RESP_PACKET_SIZE (sizeof(ethernet_hdr_t) + sizeof(ComMessageHdr) + sizeof(PdelayRespMessage))
  unsigned int buf0[(PDELAY_RESP_PACKET_SIZE+3)/4];
  unsigned char *buf = (unsigned char *) &buf0[0];
  // received packet pointers.
  ComMessageHdr *pRxMesgHdr = (ComMessageHdr *) pdelay_req_msg;   
  // transmit packet pointers.
  ComMessageHdr *pTxMesgHdr = (ComMessageHdr *) &buf[sizeof(ethernet_hdr_t)]; 
  PdelayRespMessage *pTxRespHdr = 
   (PdelayRespMessage *) &buf[sizeof(ethernet_hdr_t) + sizeof(ComMessageHdr)];
  PdelayRespFollowUpMessage *pTxFollowUpHdr = 
   (PdelayRespFollowUpMessage *) &buf[sizeof(ethernet_hdr_t) + sizeof(ComMessageHdr)];

  ptp_timestamp epoch_req_ingress_ts;
  ptp_timestamp epoch_resp_ts;
  unsigned local_resp_ts;

  set_ptp_ethernet_hdr(buf);

  memset(pTxMesgHdr, 0, sizeof(ComMessageHdr) + sizeof(PdelayRespMessage));

  pTxMesgHdr->versionPTP = PTP_VERSION_NUMBER;

  pTxMesgHdr->messageLength = hton16(sizeof(ComMessageHdr) +
                                     sizeof(PdelayRespMessage));

  pTxMesgHdr->sourcePortIdentity = my_port_id;

  pTxMesgHdr->controlField = PTP_CTL_FIELD_OTHERS;
  pTxMesgHdr->logMessageInterval = PTP_LOG_MIN_PDELAY_REQ_INTERVAL;

  pTxMesgHdr->sequenceId = pRxMesgHdr->sequenceId;

  pTxRespHdr->requestingPortIdentity = pRxMesgHdr->sourcePortIdentity;

  pTxMesgHdr->domainNumber = pRxMesgHdr->domainNumber;

  pTxMesgHdr->correctionField = pRxMesgHdr->correctionField;

  /* Send the response message */

  pTxMesgHdr->transportSpecific_messageType = 
    PTP_TRANSPORT_SPECIFIC_HDR | PTP_PDELAY_RESP_MESG;

  local_to_epoch_ts(req_ingress_ts, &epoch_req_ingress_ts);

  timestamp_to_network(&pTxRespHdr->requestReceiptTimestamp,
                       &epoch_req_ingress_ts);
                       
  ptp_tx_timed(c_tx,  buf0, PDELAY_RESP_PACKET_SIZE, &local_resp_ts);

  /* Now send the follow up */

  local_to_epoch_ts(local_resp_ts, &epoch_resp_ts);

  pTxMesgHdr->transportSpecific_messageType = 
    PTP_TRANSPORT_SPECIFIC_HDR | PTP_PDELAY_RESP_FOLLOW_UP_MESG;

  timestamp_to_network(&pTxFollowUpHdr->responseOriginTimestamp,
                       &epoch_resp_ts);

  ptp_tx(c_tx, buf0, PDELAY_RESP_PACKET_SIZE);

  return;
}



static unsigned received_sync = 0;
static u16_t received_sync_id;
static unsigned received_sync_ts;

static unsigned received_pdelay = 0;
static u16_t received_pdelay_id;
static unsigned pdelay_resp_ingress_ts;
static ptp_timestamp pdelay_request_receipt_ts;


void ptp_recv(chanend c_tx,
              char *buf, 
              unsigned local_ingress_ts) 
{

  /* Extract the ethernet header and ptp common message header */
  struct ethernet_hdr_t *ethernet_hdr = (ethernet_hdr_t *) &buf[0];
  int has_qtag = ethernet_hdr->ethertype[1]==0x18;
  int ethernet_pkt_size = has_qtag ? 18 : 14;
  ComMessageHdr *msg =  (ComMessageHdr *) &buf[ethernet_pkt_size];

  local_ingress_ts = local_ingress_ts - tile_timer_offset;

  switch ((msg->transportSpecific_messageType & 0xf)) 
    {
    case PTP_ANNOUNCE_MESG: {
        AnnounceMessage *announce_msg = (AnnounceMessage *) (msg + 1);

      bmca_update_roles((char *) msg, local_ingress_ts);
      
      if (ptp_state == PTP_SLAVE && 
          port_id_equal(&master_port_id, &msg->sourcePortIdentity) &&
          clock_id_equal(&best_announce_msg.grandmasterIdentity,
                         &announce_msg->grandmasterIdentity)) {
        last_received_announce_time_valid = 1;
        last_received_announce_time = local_ingress_ts;
      }
      }
      break;
    case PTP_SYNC_MESG:

      if (port_id_equal(&master_port_id, &msg->sourcePortIdentity)) {
        received_sync = 1;
        received_sync_id = ntoh16(msg->sequenceId);        
        received_sync_ts = local_ingress_ts;
      }
      break;
    case PTP_FOLLOW_UP_MESG:    
      if (received_sync &&
          received_sync_id == ntoh16(msg->sequenceId) &&
          port_id_equal(&master_port_id, &msg->sourcePortIdentity)) {
        FollowUpMessage *follow_up_msg = (FollowUpMessage *) (msg + 1);
        ptp_timestamp master_egress_ts;
        long long correction;

        correction = ntoh64(msg->correctionField);

        network_to_ptp_timestamp(&master_egress_ts,
                                 &follow_up_msg->preciseOriginTimestamp);

        ptp_timestamp_offset64(&master_egress_ts, &master_egress_ts, 
                               correction>>16);
                
        update_adjust(&master_egress_ts,received_sync_ts);
        update_reference_timestamps(&master_egress_ts, received_sync_ts);
      }
      received_sync = 0;
      break;
    case PTP_PDELAY_REQ_MESG:
      if (!ptp_master_only_pdelay_resp || ptp_state == PTP_MASTER)
        {
          send_ptp_pdelay_resp_msg(c_tx, (char *) msg, local_ingress_ts);
        }
      break;
    case PTP_PDELAY_RESP_MESG:

      if (ptp_legacy_mode == 0 ||
          port_id_equal(&master_port_id, &msg->sourcePortIdentity)) {
        if (pdelay_request_sent && 
            pdelay_req_seq_id == ntoh16(msg->sequenceId)) {
          PdelayRespMessage *resp_msg = (PdelayRespMessage *) (msg + 1);
          received_pdelay = 1;
          received_pdelay_id = ntoh16(msg->sequenceId);        
          pdelay_resp_ingress_ts = local_ingress_ts;        
          network_to_ptp_timestamp(&pdelay_request_receipt_ts,
                                   &resp_msg->requestReceiptTimestamp);
        }
        pdelay_request_sent = 0;
      }
      break;
    case PTP_PDELAY_RESP_FOLLOW_UP_MESG:  
      if (ptp_legacy_mode == 0 || 
          port_id_equal(&master_port_id, &msg->sourcePortIdentity)) {
        if (received_pdelay && received_pdelay_id == ntoh16(msg->sequenceId)) {
          ptp_timestamp pdelay_resp_egress_ts;
          PdelayRespFollowUpMessage *follow_up_msg = 
            (PdelayRespFollowUpMessage *) (msg + 1);
          
          network_to_ptp_timestamp(&pdelay_resp_egress_ts,
                                   &follow_up_msg->responseOriginTimestamp);
          
          update_path_delay(&pdelay_request_receipt_ts,
                            &pdelay_resp_egress_ts,
                            pdelay_request_sent_ts,
                            pdelay_resp_ingress_ts);
          
        }
        received_pdelay = 0;
      }
      break;
    }     
}

#ifdef GPTP_DEBUG
static unsigned last_debug_time;
#endif

void ptp_init(chanend c_tx, chanend c_rx, enum ptp_server_type stype)
{
  unsigned t;
  mac_get_tile_timer_offset(c_rx, &tile_timer_offset);
  t = get_local_time();


#ifdef GPTP_DEBUG  
  last_debug_time = t;
#endif


  if (stype == PTP_GRANDMASTER_CAPABLE) {
    ptp_priority1 = PTP_DEFAULT_GM_CAPABLE_PRIORITY1;
  }
  else {
    ptp_priority1 = PTP_DEFAULT_NON_GM_CAPABLE_PRIORITY1;
  }

  mac_get_macaddr(c_tx, src_mac_addr);

  my_port_id.data[0] = src_mac_addr[0];
  my_port_id.data[1] = src_mac_addr[1];
  my_port_id.data[2] = src_mac_addr[2];
  my_port_id.data[3] = 0xff;
  my_port_id.data[4] = 0xfe;
  my_port_id.data[5] = src_mac_addr[3];
  my_port_id.data[6] = src_mac_addr[4];
  my_port_id.data[7] = src_mac_addr[5];
  my_port_id.data[8] = 0;
  my_port_id.data[9] = 1;

  set_new_role(PTP_MASTER, t);

  pdelay_epoch_timer = t;
}


void ptp_server_set_legacy_mode(int legacy_mode)
{
  if (legacy_mode) {
    memcpy(dest_mac_addr, legacy_dest_mac_addr, sizeof(dest_mac_addr));
    ptp_master_only_pdelay_resp = 1;
  }
  else {
    memcpy(dest_mac_addr, non_legacy_dest_mac_addr,sizeof(dest_mac_addr));
    ptp_master_only_pdelay_resp = 0;
  }
  ptp_legacy_mode = legacy_mode;
}

void ptp_periodic(chanend c_tx, unsigned t) {

  if (last_received_announce_time_valid && 
      timeafter(t, last_received_announce_time + RECV_ANNOUNCE_TIMEOUT)) {

    if (ptp_state == PTP_SLAVE ) {
      set_new_role(PTP_UNCERTAIN, t);
      last_received_announce_time = t;
      last_announce_time = t - ANNOUNCE_PERIOD - 1;
    }
    else
      if (ptp_state == PTP_UNCERTAIN) {
        set_new_role(PTP_MASTER, t);
      }
  }

  if ((ptp_state == PTP_MASTER || ptp_state == PTP_UNCERTAIN
       || ptp_state == PTP_SLAVE) && 
      timeafter(t, last_announce_time + ANNOUNCE_PERIOD)) {
    send_ptp_announce_msg(c_tx);
    last_announce_time = t;
  }

  if (ptp_state == PTP_MASTER && 
      timeafter(t, last_sync_time + SYNC_PERIOD)) {
    send_ptp_sync_msg(c_tx);
    last_sync_time = t;
  }

  if ((ptp_legacy_mode == 0 || ptp_state == PTP_SLAVE || ptp_state == PTP_UNCERTAIN) &&
      (timeafter(t, last_pdelay_req_time + PDELAY_REQ_PERIOD))) {
    send_ptp_pdelay_req_msg(c_tx);
    last_pdelay_req_time = t;
  }

  periodic_update_reference_timestamps(t);
  return;
}

void ptp_current_grandmaster(char grandmaster[8])
{
	int i;
	memcpy(grandmaster, best_announce_msg.grandmasterIdentity.data, 8);
}
