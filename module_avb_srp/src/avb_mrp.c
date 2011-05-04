#include <xs1.h>
#include "avb_mrp.h"
#include "avb_srp.h"
#include "avb_mmrp.h"
#include "avb_mvrp.h"
#include "avb_mrp_pdu.h"
#include "avb_mmrp_pdu.h"
#include "avb_mvrp_pdu.h"
#include "avb_srp_pdu.h"
#include "misc_timer.h"
#include "ethernet_tx_client.h"
#include <print.h>
#include "simple_printf.h"
#include "avb_internal.h"
#include <string.h>



#define TICKS_PER_CENTISECOND (XS1_TIMER_KHZ * 10)

#define MAX_MRP_MSG_SIZE (sizeof(mrp_msg_header) + sizeof(srp_talker_first_value) + 1 /* for event vector */ + sizeof(mrp_msg_footer))

static int first_value_lengths[MRP_NUM_ATTRIBUTE_TYPES] = FIRST_VALUE_LENGTHS;

static unsigned char mmrp_dest_mac[6] = AVB_MMRP_MACADDR;
static unsigned char mvrp_dest_mac[6] = AVB_MVRP_MACADDR;
static unsigned char srp_proper_dest_mac[6] = AVB_SRP_MACADDR;
static unsigned char srp_legacy_dest_mac[6] = AVB_SRP_LEGACY_MACADDR;
static char send_buf[1518];

static mrp_attribute_state attrs[MRP_MAX_ATTRS];
static mrp_attribute_state *first_attr = &attrs[0];

static char *send_ptr= &send_buf[0] + sizeof(mrp_ethernet_hdr) + sizeof(mrp_header);
static int current_etype = 0;
static int legacy_mode = 0;


static void configure_send_buffer_msrp() {
  mrp_ethernet_hdr* hdr = (mrp_ethernet_hdr *) &send_buf[0];

  if (legacy_mode) 
    memcpy(&hdr->dest_addr, srp_legacy_dest_mac, 6);
  else
    memcpy(&hdr->dest_addr, srp_proper_dest_mac, 6);
  
  hdr->ethertype[0] = (AVB_SRP_ETHERTYPE >> 8);
  hdr->ethertype[1] = AVB_SRP_ETHERTYPE & 0xff;
  current_etype = AVB_SRP_ETHERTYPE;
}

static void configure_send_buffer_mmrp() {
  mrp_ethernet_hdr* hdr = (mrp_ethernet_hdr *) &send_buf[0];

  memcpy(&hdr->dest_addr, mmrp_dest_mac, 6);
  
  hdr->ethertype[0] = (AVB_MMRP_ETHERTYPE >> 8);
  hdr->ethertype[1] = AVB_MMRP_ETHERTYPE & 0xff;
  current_etype = AVB_MMRP_ETHERTYPE;
}

static void configure_send_buffer_mvrp() {
  mrp_ethernet_hdr* hdr = (mrp_ethernet_hdr *) &send_buf[0];

  memcpy(&hdr->dest_addr, mvrp_dest_mac, 6);
  
  hdr->ethertype[0] = (AVB_MVRP_ETHERTYPE >> 8);
  hdr->ethertype[1] = AVB_MVRP_ETHERTYPE & 0xff;
  current_etype = AVB_MVRP_ETHERTYPE;
}

void avb_mrp_set_legacy_mode(int mode)
{
  legacy_mode = 1;
} 


static void force_send(chanend c_tx)
{
  char *buf = &send_buf[0];
  char *ptr = send_ptr;
  if (ptr != buf+sizeof(mrp_ethernet_hdr)+sizeof(mrp_header)) {
    char *end = ptr + 4;
    if (end < buf + 64)
      end = buf + 64;
    for (char *p = ptr;p<end;p++)
      *p = 0;
    mac_tx(c_tx, (unsigned int *) buf, end - buf, ETH_BROADCAST);
  }
  send_ptr = buf+sizeof(mrp_ethernet_hdr)+sizeof(mrp_header);
  return;
}

static void send(chanend c_tx)
{
  int max_msg_size = MAX_MRP_MSG_SIZE;
  int bufsize = 1518;
  char *buf = &send_buf[0];
  char *ptr = send_ptr;
  if (buf + bufsize < ptr + max_msg_size + sizeof(mrp_footer)) {
    force_send(c_tx);
  }
}





#define timeafter(A, B) ((int)((B) - (A)) < 0)


static mrp_timer periodic_timer;
static mrp_timer joinTimer;

#ifdef MRP_FULL_PARTICIPANT
static mrp_timer leaveall_timer;
#endif



static inline void init_timer(mrp_timer *tmr,
                              int mult)
{
  tmr->active = 0;
  tmr->timeout_multiplier = mult;
}

static inline void start_timer(mrp_timer *tmr,
                             unsigned int period_cs) 
{
  tmr->period = (period_cs * TICKS_PER_CENTISECOND);
  tmr->timeout = get_local_time() + (period_cs * TICKS_PER_CENTISECOND);
  tmr->active = tmr->timeout_multiplier;
} 

static inline int timer_expired(mrp_timer *tmr)
{
  unsigned int now = get_local_time();
  if (!tmr->active) 
    return 0;

  if (timeafter(now, tmr->timeout)) {
    tmr->active--;
    tmr->timeout = now + tmr->period;
  }
  
  return (tmr->active == 0);
}

static inline void stop_timer(mrp_timer *tmr)
{
  tmr->active = 0;
}
 
static unsigned int makeTxEvent(mrp_event e, mrp_attribute_state *st, int leave_all)
{
  int firstEvent = 0;
  switch (st->applicant_state) 
    {
    case MRP_VP:
    case MRP_AA:
    case MRP_AP:
    case MRP_QA:
    case MRP_QP:
      // sJ
#ifdef MRP_FULL_PARTICIPANT
      if (leave_all && st->applicant_state == MRP_VP) {
        switch (st->registrar_state) 
          {
          case MRP_IN:
            firstEvent = MRP_ATTRIBUTE_EVENT_IN;
            break;
          default:
            firstEvent = MRP_ATTRIBUTE_EVENT_MT;
          break;
          }
      }
      else
      if (leave_all || st->applicant_state != MRP_QP) {
        switch (st->registrar_state) 
          {
          case MRP_IN:
            firstEvent = MRP_ATTRIBUTE_EVENT_JOININ;
            break;
          default:
            firstEvent = MRP_ATTRIBUTE_EVENT_JOINMT;
            break;
          }
      }
#else
      firstEvent = MRP_ATTRIBUTE_EVENT_JOININ;
#endif      
      break;
    case MRP_VN:
    case MRP_AN:
      //sN
      firstEvent = MRP_ATTRIBUTE_EVENT_NEW;
      //      __asm__("ecallf %0"::"r"(0));
      break;
    case MRP_LA:
      //sL
      firstEvent = MRP_ATTRIBUTE_EVENT_LV;
      break;  
#ifdef MRP_FULL_PARTICIPANT
    case MRP_LO:
      //s
      switch (st->registrar_state) 
        {
        case MRP_IN:
          firstEvent = MRP_ATTRIBUTE_EVENT_IN;
          break;
        default:
          firstEvent = MRP_ATTRIBUTE_EVENT_MT;
          break;
        }
      break;           
#endif
    }
  return firstEvent;
}


int static decode_attr_type(int etype, int atype) {
  switch (etype) 
    {
    case AVB_SRP_ETHERTYPE:
      switch (atype) 
        {
        case AVB_SRP_ATTRIBUTE_TYPE_TALKER_ADVERTISE:
          return MSRP_TALKER_ADVERTISE;
        case AVB_SRP_ATTRIBUTE_TYPE_TALKER_FAILED:
          return MSRP_TALKER_FAILED;
        case AVB_SRP_ATTRIBUTE_TYPE_LISTENER:
          return MSRP_LISTENER;
        case AVB_SRP_ATTRIBUTE_TYPE_DOMAIN:
          return MSRP_DOMAIN_VECTOR;
        }
      break;
    case AVB_MMRP_ETHERTYPE:
      switch (atype) 
        {
        case AVB_MMRP_ATTRIBUTE_TYPE:
          return MMRP_MAC_VECTOR;
        }
      break;
    case AVB_MVRP_ETHERTYPE:
      switch (atype) 
        {
        case AVB_MVRP_ATTRIBUTE_TYPE:
          return MVRP_VID_VECTOR;
        }
      break;   
  }
  return -1;
}


static int encode_attr_type(mrp_attribute_type attr)
{
  switch (attr) {
  case MSRP_TALKER_ADVERTISE:
    return AVB_SRP_ATTRIBUTE_TYPE_TALKER_ADVERTISE;
    break;
  case MSRP_TALKER_FAILED:
    return AVB_SRP_ATTRIBUTE_TYPE_TALKER_FAILED;
    break;
  case MSRP_LISTENER:
    return AVB_SRP_ATTRIBUTE_TYPE_LISTENER;
    break;
  case MSRP_DOMAIN_VECTOR:
    return AVB_SRP_ATTRIBUTE_TYPE_DOMAIN;
    break;
  case MMRP_MAC_VECTOR:
    return AVB_MMRP_ATTRIBUTE_TYPE;
    break;
  case MVRP_VID_VECTOR:
    return AVB_MVRP_ATTRIBUTE_TYPE;
    break;
  default:
    return 0;
  }
}

static int has_fourpacked_events(mrp_attribute_type attr) {
  switch (attr) {  
  case MSRP_LISTENER:
    return 1;
    break;
  default:
    return 0;
    break;
  }
  return 0;
}

static int encode_three_packed(int event, int i, int vector)
{
  
  for (int j=0;j<(2-i);j++)
    event *= 6;
  return (vector + event);
}

void mrp_encode_three_packed_event(char *buf,
                                   int event,
                                   mrp_attribute_type attr)
{
  mrp_msg_header *hdr = (mrp_msg_header *) buf;  
  mrp_vector_header *vector_hdr = (mrp_vector_header *) (buf + sizeof(mrp_msg_header));  
  int num_values = vector_hdr->NumberOfValuesLow;
  int first_value_length =  first_value_lengths[attr];  
  char *vector = buf + sizeof(mrp_msg_header) + sizeof(mrp_vector_header) + first_value_length + num_values/3;
  int shift_required = (num_values % 3 == 0);
  unsigned attr_list_length = (hdr->AttributeListLength[0]<<8) + 
    hdr->AttributeListLength[1];



  if (shift_required) {
    char *endmark;
    if (send_ptr - vector > 0) 
      memmove(vector+1, vector, send_ptr - vector);
    send_ptr++;
    *vector = 0;
    attr_list_length++;
    HTON_U16(hdr->AttributeListLength, attr_list_length);           
    endmark = buf + sizeof(mrp_msg_header) + attr_list_length - 2;
    *endmark = 0;
    *(endmark+1) = 0;
  }
  
  *vector = encode_three_packed(event, num_values % 3, *vector);
  return;
}


static int encode_four_packed(int event, int i, int vector)
{
  
  for (int j=0;j<(3-i);j++)
    event *= 4;
  return (vector + event);
}

void mrp_encode_four_packed_event(char *buf,
                                  int event,
                                  mrp_attribute_type attr)
{
  mrp_msg_header *hdr = (mrp_msg_header *) buf;  
  mrp_vector_header *vector_hdr = (mrp_vector_header *) (buf + sizeof(mrp_msg_header));  
  int num_values = vector_hdr->NumberOfValuesLow;
  int first_value_length =  first_value_lengths[attr];  
  char *vector = buf + sizeof(mrp_msg_header) + sizeof(mrp_vector_header) + first_value_length + (num_values+3)/3 + num_values/4 ;
  int shift_required = (num_values % 4 == 0);
  unsigned attr_list_length = (hdr->AttributeListLength[0]<<8) + 
    hdr->AttributeListLength[1];



  if (shift_required)  {
    char *endmark;
    if (send_ptr - vector > 0) 
      memmove(vector+1, vector, send_ptr - vector);
    *vector = 0;
    attr_list_length++;
    send_ptr++;
    HTON_U16(hdr->AttributeListLength, attr_list_length);
    endmark = buf + sizeof(mrp_msg_header) + attr_list_length - 2;
    *endmark = 0;
    *(endmark+1) = 0;
  }
  
  *vector = encode_four_packed(event, num_values % 4, *vector);
  return;
}

// Send an empty leave all message
// This may be merged later with a redeclaration if we have 
// declaration for this attribute
static void create_empty_msg(mrp_attribute_type attr, int leave_all) {
  mrp_msg_header *hdr = (mrp_msg_header *) send_ptr;  
  mrp_vector_header *vector_hdr = (mrp_vector_header *) (send_ptr + sizeof(mrp_msg_header));
  int hdr_length = sizeof(mrp_msg_header);
  //  int vector_length = has_fourpacked_events(attr) ? 2 : 1;
  int vector_length = 0;
  int first_value_length =  first_value_lengths[attr];
  int attr_list_length = first_value_length + sizeof(mrp_vector_header)  + vector_length + sizeof(mrp_footer);
  int msg_length = hdr_length + attr_list_length;

  // clear message
  memset((char *)hdr, 0, msg_length);

  // Set the relevant fields
  hdr->AttributeType = encode_attr_type(attr);
  hdr->AttributeLength = first_value_length;
  HTON_U16(hdr->AttributeListLength, attr_list_length);
  
  vector_hdr->LeaveAllEventNumberOfValuesHigh = leave_all << 5;
  vector_hdr->NumberOfValuesLow = 0;

  send_ptr += msg_length;
}


static int merge_msg(char *msg, mrp_attribute_state* st, int vector)
{
  switch (st->attribute_type) 
    {
    case MSRP_TALKER_ADVERTISE: 
    case MSRP_TALKER_FAILED: 
    case MSRP_LISTENER:
    case MSRP_DOMAIN_VECTOR:             
      return avb_srp_merge_message(msg, st, vector);
      break;
    case MMRP_MAC_VECTOR:
      return avb_mmrp_merge_message(msg, st, vector);
      break;
    case MVRP_VID_VECTOR:
      return avb_mvrp_merge_message(msg, st, vector);
      break;
  }
  return 0;
}

static void doTx(mrp_attribute_state *st,
                 int vector)
{
  chanend c_tx = avb_control_get_mac_tx();
  int merged = 0;
  char *msg = &send_buf[0]+sizeof(mrp_ethernet_hdr)+sizeof(mrp_header);
  char *end = send_ptr;

  while (!merged &&
         msg < end && 
         (*msg != 0 || *(msg+1) != 0)) {      
    mrp_msg_header *hdr = (mrp_msg_header *) &msg[0];     
    unsigned attr_list_len;

    merged = merge_msg(msg, st, vector);

    attr_list_len = 
      (hdr->AttributeListLength[0]<<8) + 
      hdr->AttributeListLength[1];
    
    msg = msg + sizeof(mrp_msg_header) + attr_list_len;
  }   

  if (!merged) {
    create_empty_msg(st->attribute_type, 0);
    (void) merge_msg(msg, st, vector);
  }

  send(c_tx);
}

static void send_join_indication(mrp_attribute_state *st, int new)
{
  switch (st->attribute_type)
  {
  case MSRP_TALKER_ADVERTISE:
	  avb_srp_talker_join_ind(st, new);
	  break;
  case MSRP_TALKER_FAILED:
	  break;
  case MSRP_LISTENER:
	  avb_srp_listener_join_ind(st, new);
	  break;
  }
}

static void send_leave_indication(mrp_attribute_state *st)
{
  switch (st->attribute_type)
  {
  case MSRP_TALKER_ADVERTISE:
	  avb_srp_talker_leave_ind(st);
	  break;
  case MSRP_TALKER_FAILED:
	  break;
  case MSRP_LISTENER:
	  avb_srp_listener_leave_ind(st);
	  break;
  }
}

static void mrp_update_state(mrp_event e, mrp_attribute_state *st)
{
#ifdef MRP_FULL_PARTICIPANT
  // Registrar state machine
  switch (e) 
    {
    case MRP_EVENT_BEGIN:
      st->registrar_state = MRP_MT;
      break;
    case MRP_EVENT_RECEIVE_NEW:
      if (st->registrar_state == MRP_LV) 
        stop_timer(&st->leaveTimer);
      st->registrar_state = MRP_IN;
      send_join_indication(st, 1);
      break;
    case MRP_EVENT_RECEIVE_JOININ:
    case MRP_EVENT_RECEIVE_JOINMT:
      if (st->registrar_state == MRP_LV)
        stop_timer(&st->leaveTimer);
      if (st->registrar_state == MRP_MT) {
          send_join_indication(st, 0);
      }
      st->registrar_state = MRP_IN;
      break;
    case MRP_EVENT_RECEIVE_LEAVE:
    case MRP_EVENT_RECEIVE_LEAVE_ALL:
    case MRP_EVENT_TX_LEAVE_ALL:
    case MRP_EVENT_REDECLARE:
      if (st->registrar_state == MRP_IN) {
        start_timer(&st->leaveTimer, MRP_LEAVETIMER_PERIOD_CENTISECONDS);
        st->registrar_state = MRP_LV;
      }
      break;
    case MRP_EVENT_LEAVETIMER:
    case MRP_EVENT_FLUSH:
      if (st->registrar_state == MRP_LV) {
        // Lv        send_leave_indication(st);
      }
      st->registrar_state = MRP_MT;
      break;
    default:
      break;
    }
#endif

  // Applicant state machine
  switch (e) 
    {
    case MRP_EVENT_BEGIN:
      st->applicant_state = MRP_VO;
      break;
    case MRP_EVENT_NEW:
      st->applicant_state = MRP_VN;
      break;
    case MRP_EVENT_JOIN:
      switch (st->applicant_state) 
        {
        case MRP_VO:
#ifdef MRP_FULL_PARTICIPANT
        case MRP_LO:
#endif
          st->applicant_state = MRP_VP;
          break;
        case MRP_LA:
          st->applicant_state = MRP_AA;
          break;
        case MRP_AO:
          st->applicant_state = MRP_AP;
          break;
        case MRP_QO:
          st->applicant_state = MRP_QP;
          break;      
        }
      break;
    case MRP_EVENT_LV:
      switch (st->applicant_state) 
        {
        case MRP_QP:
          st->applicant_state = MRP_QO;
          break;
        case MRP_AP:
          st->applicant_state = MRP_AO;
          break;
        case MRP_VP:
          st->applicant_state = MRP_VO;
          break;
        case MRP_VN:
        case MRP_AN:
        case MRP_AA:
        case MRP_QA:
          st->applicant_state = MRP_LA;
          break;
        }
      break;
    case MRP_EVENT_RECEIVE_JOININ:
      switch (st->applicant_state) 
        {
        case MRP_VO:
          st->applicant_state = MRP_AO;
          break;
        case MRP_VP:
          st->applicant_state = MRP_AP;
          break;
        case MRP_AA:
          st->applicant_state = MRP_QA;
          break;
        case MRP_AO:
          st->applicant_state = MRP_QO;
          break;
        case MRP_AP:
          st->applicant_state = MRP_QP;
          break;
        }
    case MRP_EVENT_RECEIVE_IN:
    	switch (st->applicant_state)
    	{
    	case MRP_AA:
    	  st->applicant_state = MRP_QA;
    	  break;
    	}
    case MRP_EVENT_RECEIVE_JOINMT:
    case MRP_EVENT_RECEIVE_MT:
      switch (st->applicant_state) 
        {
        case MRP_QA:
          st->applicant_state = MRP_AA;
          break;
        case MRP_QO:
          st->applicant_state = MRP_AO;
          break;
        case MRP_QP:
          st->applicant_state = MRP_AP;
          break;
#ifdef MRP_FULL_PARTICIPANT
        case MRP_LO:
          st->applicant_state = MRP_VO;
          break;
#endif
        }      
      break;
    case MRP_EVENT_RECEIVE_LEAVE:
    case MRP_EVENT_RECEIVE_LEAVE_ALL:
    case MRP_EVENT_REDECLARE:
      switch (st->applicant_state) 
        {
        case MRP_VO:
        case MRP_AO:
        case MRP_QO:
#ifdef MRP_FULL_PARTICIPANT
          st->applicant_state = MRP_LO;
#else
          st->applicant_state = MRP_VO;
#endif
          break;
        case MRP_AN:
          st->applicant_state = MRP_VN;
          break;
        case MRP_AA:
        case MRP_QA:
        case MRP_AP:
        case MRP_QP:
          st->applicant_state = MRP_VP;
          break;
        }      
      break;
    case MRP_EVENT_PERIODIC:
      switch (st->applicant_state) 
        {
        case MRP_QA:
          st->applicant_state = MRP_AA;
          break;
        case MRP_QP:
          st->applicant_state = MRP_AP;
          break;
        }
      break;
    case MRP_EVENT_TX: 
      switch (st->applicant_state) 
        {
        case MRP_VP:
        case MRP_VN:
        case MRP_AN:
        case MRP_AA:
        case MRP_LA:
        case MRP_AP:
#ifdef MRP_FULL_PARTICIPANT
        case MRP_LO:
#endif
          {
          int vector = makeTxEvent(e, st, 0);
          doTx(st, vector);
          break;
          }
        }
      switch (st->applicant_state) 
        {
        case MRP_VP:
          st->applicant_state = MRP_AA;
          break;
        case MRP_VN:
          st->applicant_state = MRP_AN;
          break;
        case MRP_AN:
        case MRP_AA:
        case MRP_AP:
          st->applicant_state = MRP_QA;
          break;
        case MRP_LA:
#ifdef MRP_FULL_PARTICIPANT
        case MRP_LO:
#endif
          st->applicant_state = MRP_VO;
          break;          
        }
      break;
#ifdef MRP_FULL_PARTICIPANT
    case MRP_EVENT_TX_LEAVE_ALL: {
      switch (st->applicant_state) 
        {
        case MRP_VP:
        case MRP_VN:
        case MRP_AN:
        case MRP_AA:
        case MRP_LA:
        case MRP_QA:
        case MRP_AP:
        case MRP_QP:
          {         
          int vector = makeTxEvent(e, st, 1);
          doTx(st, vector);
        }
        }
      switch (st->applicant_state) 
        {
        case MRP_VO:
        case MRP_LA:
        case MRP_AO:
        case MRP_QO:
          st->applicant_state = MRP_LO;
          break;
        case MRP_VN:
          st->applicant_state = MRP_AN;
          break;
        case MRP_AN:
        case MRP_AA:
        case MRP_AP:
        case MRP_QP:
          st->applicant_state = MRP_QA;
          break;
        }
      }
      break;
#endif
    default:
      break;
    }

  //  simple_printf("Update state out: %d\n", st->applicant_state);
}



void mrp_attribute_init(mrp_attribute_state *st,
                        mrp_attribute_type t,
                        void *info)
{
  st->attribute_type = t;
  st->attribute_info = info;
  return;
}


void mrp_mad_begin(mrp_attribute_state *st)
{
#ifdef MRP_FULL_PARTICIPANT
  init_timer(&st->leaveTimer, 1);
#endif
  mrp_update_state(MRP_EVENT_BEGIN, st);
}

void mrp_mad_join(mrp_attribute_state *st, int new)
{
  if (new) {
    mrp_update_state(MRP_EVENT_NEW, st);
  } else {
    mrp_update_state(MRP_EVENT_JOIN, st);
  }
}

void mrp_mad_leave(mrp_attribute_state *st)
{
  mrp_update_state(MRP_EVENT_LV, st);
}

void mrp_init(char *macaddr)
{
  for (int i=0;i<6;i++) {
    mrp_ethernet_hdr *hdr = (mrp_ethernet_hdr *) &send_buf[0];
    hdr->src_addr[i] = macaddr[i];
  }

  for (int i=0;i<MRP_MAX_ATTRS;i++) {
    attrs[i].applicant_state = MRP_UNUSED;
    if (i != MRP_MAX_ATTRS-1) 
      attrs[i].next = &attrs[i+1];
    else
      attrs[i].next = NULL;
  }
  first_attr = &attrs[0];

  init_timer(&periodic_timer, MRP_PERIODIC_TIMER_MULTIPLIER);
  start_timer(&periodic_timer, MRP_PERIODIC_TIMER_PERIOD_CENTISECONDS / MRP_PERIODIC_TIMER_MULTIPLIER);

  init_timer(&joinTimer, 1);
  start_timer(&joinTimer, MRP_JOINTIMER_PERIOD_CENTISECONDS);


#ifdef MRP_FULL_PARTICIPANT
  init_timer(&leaveall_timer, MRP_LEAVEALL_TIMER_MULTIPLIER);
  start_timer(&leaveall_timer, MRP_LEAVEALL_TIMER_PERIOD_CENTISECONDS / MRP_LEAVEALL_TIMER_MULTIPLIER);
#endif

}

static int compare_attr(mrp_attribute_state *a,
                        mrp_attribute_state *b)
{
  if (a->applicant_state == MRP_UNUSED) {
    if (b->applicant_state == MRP_UNUSED) 
      return (a < b);   
    else
      return 0;
  }
  else if (b->applicant_state == MRP_UNUSED) {
    return 1;
  }


  if (a->applicant_state == MRP_DISABLED) {
    if (b->applicant_state == MRP_DISABLED) 
      return (a < b);   
    else
      return 0;
  }
  else if (b->applicant_state == MRP_DISABLED) {
    return 1;
  }

  
  if (a->attribute_type != b->attribute_type)
    return (a->attribute_type < b->attribute_type);

  switch (a->attribute_type) 
    {
    case MSRP_TALKER_ADVERTISE:
      return avb_srp_compare_talker_attributes(a,b);
      break;
    case MSRP_LISTENER:
      return avb_srp_compare_listener_attributes(a,b);
      break;    
    default:
      break;
    }
  return (a<b);
}

mrp_attribute_state *mrp_get_attr(void)
{
  for (int i=0;i<MRP_MAX_ATTRS;i++) {
    if (attrs[i].applicant_state == MRP_UNUSED) {      
      attrs[i].applicant_state = MRP_DISABLED;
      return &attrs[i];
    }
  }
  return NULL;
}

static void sort_attrs()
{
  mrp_attribute_state *to_insert=NULL;
  mrp_attribute_state *attr=NULL;
  mrp_attribute_state *prev=NULL;

  // This sorting algorithm is designed to work best for lists
  // we already expect to be sorted (which is generally the case here)

  // Get items to insert back in  
  attr = first_attr;
  while (attr != NULL) {
    mrp_attribute_state *next = attr->next;
    if (next != NULL) {
      if (!compare_attr(attr, next)) {
        if (prev) 
          prev->next = next;                      
        else 
          first_attr = next;
        
        attr->next = to_insert;
        to_insert = attr;
      }
      else 
        prev = attr;
    }   
    attr = next;
  }

  // Inser them back in
  attr = to_insert;
  while (attr != NULL) {
    mrp_attribute_state *next = attr->next;
    mrp_attribute_state *ins = first_attr;

    if (compare_attr(attr, ins)) {
      attr->next = ins;
      first_attr = attr;
    }
    else {      
      while (ins != NULL) {
        mrp_attribute_state *ins_next = ins->next;
        if (ins_next == NULL || 
            (compare_attr(ins, attr) && compare_attr(attr, ins_next))) {
          attr->next = ins->next;
          ins->next = attr;
          ins_next = NULL;
        }
        ins = ins_next;
      }
    }
    attr = next;
  }
  

}

static void global_event(mrp_event e) {
  mrp_attribute_state *attr = first_attr;
  while (attr != NULL) {
    if (attr->applicant_state != MRP_DISABLED &&
        attr->applicant_state != MRP_UNUSED) 
      mrp_update_state(e, attr);
    attr = attr->next;
  }
  
}

static void attribute_type_event(mrp_attribute_type atype, mrp_event e) {
  mrp_attribute_state *attr = first_attr;
  while (attr != NULL) {
    if (attr->applicant_state != MRP_DISABLED && 
        attr->applicant_state != MRP_UNUSED && 
        attr->attribute_type == atype)  {
      mrp_update_state(e, attr);
    }
    attr = attr->next;
  }
}




void mrp_periodic()
{
  chanend c_tx = avb_control_get_mac_tx();
  static int leave_all = 0;

  if (timer_expired(&periodic_timer)) {
    global_event(MRP_EVENT_PERIODIC);    
    start_timer(&periodic_timer, MRP_PERIODIC_TIMER_PERIOD_CENTISECONDS / MRP_PERIODIC_TIMER_MULTIPLIER);
  }

#ifdef MRP_FULL_PARTICIPANT
  if (timer_expired(&leaveall_timer)) {
    start_timer(&leaveall_timer, MRP_LEAVEALL_TIMER_PERIOD_CENTISECONDS / MRP_LEAVEALL_TIMER_MULTIPLIER);
    leave_all = 1;
    global_event(MRP_EVENT_RECEIVE_LEAVE_ALL);
  }

  for (int j=0;j<MRP_MAX_ATTRS;j++) {
    if (timer_expired(&attrs[j].leaveTimer)) {
      mrp_update_state(MRP_EVENT_LEAVETIMER, &attrs[j]);
    }
  }  
#endif

  if (timer_expired(&joinTimer)) {
    mrp_event tx_event = leave_all ? MRP_EVENT_TX_LEAVE_ALL : MRP_EVENT_TX;
    start_timer(&joinTimer, MRP_JOINTIMER_PERIOD_CENTISECONDS);
    sort_attrs();    
    configure_send_buffer_msrp();
    if (leave_all) {
      create_empty_msg(MSRP_TALKER_ADVERTISE, 1);  send(c_tx);
      create_empty_msg(MSRP_TALKER_FAILED, 1);  send(c_tx);
      create_empty_msg(MSRP_LISTENER, 1);  send(c_tx);
      create_empty_msg(MSRP_DOMAIN_VECTOR, 1);  send(c_tx);
    }
    attribute_type_event(MSRP_TALKER_ADVERTISE, tx_event);
    attribute_type_event(MSRP_LISTENER, tx_event);
    attribute_type_event(MSRP_DOMAIN_VECTOR, tx_event);
    force_send(c_tx);

    configure_send_buffer_mmrp();
    if (leave_all) {
      create_empty_msg(MMRP_MAC_VECTOR, 1); send(c_tx);
    }
    attribute_type_event(MMRP_MAC_VECTOR, tx_event);
    force_send(c_tx);

    configure_send_buffer_mvrp();
    if (leave_all) {
      create_empty_msg(MVRP_VID_VECTOR, 1); send(c_tx);
    }
    attribute_type_event(MVRP_VID_VECTOR, tx_event);
    force_send(c_tx);
    leave_all = 0;
  }
}


static void mrp_in(int firstEvent, mrp_attribute_state *st)
{
  switch (firstEvent) 
    {
    case MRP_ATTRIBUTE_EVENT_NEW:
      mrp_update_state(MRP_EVENT_RECEIVE_NEW, st);
      break;
    case MRP_ATTRIBUTE_EVENT_JOININ:
      mrp_update_state(MRP_EVENT_RECEIVE_JOININ, st);
      break;
    case MRP_ATTRIBUTE_EVENT_IN:
      mrp_update_state(MRP_EVENT_RECEIVE_IN, st);
      break;
    case MRP_ATTRIBUTE_EVENT_JOINMT:
      mrp_update_state(MRP_EVENT_RECEIVE_JOINMT, st);
      break;
    case MRP_ATTRIBUTE_EVENT_MT:
      mrp_update_state(MRP_EVENT_RECEIVE_MT, st);
      break;
    case MRP_ATTRIBUTE_EVENT_LV:   
      mrp_update_state(MRP_EVENT_RECEIVE_LEAVE, st);
      break;
  }
}

int mrp_is_observer(mrp_attribute_state *st)
{
  switch (st->applicant_state) 
    {
    case MRP_VO:
    case MRP_AO:
    case MRP_QO:
      return 1;
    default:
      return 0;
    }
}



static int msg_match(mrp_attribute_type attr_type, 
              mrp_attribute_state *attr, 
              char *msg, 
              int i, 
              int three_packed_event,
              int four_packed_event)
{
  if (attr->applicant_state == MRP_UNUSED ||
      attr->applicant_state == MRP_DISABLED)
    return 0;

  if (attr->attribute_type != attr_type)
    return 0;
      
  switch (attr_type) {
  case MSRP_TALKER_ADVERTISE:   
    return avb_srp_match_talker_advertise(attr, msg, i);
  case MSRP_TALKER_FAILED:
    return avb_srp_match_talker_failed(attr, msg, i);
  case MSRP_LISTENER:
    return avb_srp_match_listener(attr, msg, i, four_packed_event);
  case MSRP_DOMAIN_VECTOR:
    return avb_srp_match_domain(attr, msg, i);
  case MMRP_MAC_VECTOR:
    return avb_mmrp_match(attr, msg, i);
  case MVRP_VID_VECTOR:
    return avb_mvrp_match(attr, msg, i);
  default:
	return 0;
  }
  return 0;
}

static void process(mrp_attribute_type attr_type, 
                   char *msg, 
                   int i, 
                   int three_packed_event,
                   int four_packed_event)
{
  switch (attr_type) {
  case MSRP_TALKER_ADVERTISE:   
  case MSRP_TALKER_FAILED:
    avb_srp_process_talker(attr_type, msg, i);
    return;
  case MSRP_LISTENER:
    avb_srp_process_listener(msg, i, four_packed_event);
    return;
  case MSRP_DOMAIN_VECTOR:
    avb_srp_process_domain(msg, i);
    return;
  case MMRP_MAC_VECTOR:
    avb_mmrp_process(msg, i);
    return;
  case MVRP_VID_VECTOR:
    avb_mvrp_process(msg, i);
    return;
  default:
	return;
  }
  return;

}


static int decode_threepacked(int vector, int i)
{
  for (int j=0;j<(2-i);j++)
    vector /= 6;
  return (vector % 6);
}

static int decode_fourpacked(int vector, int i)
{
  for (int j=0;j<(3-i);j++)
    vector /= 4;
  return (vector % 4);
}


                    

avb_status_t avb_mrp_process_packet(unsigned int buf[], int len)
{
  mrp_ethernet_hdr *hdr = (mrp_ethernet_hdr *) &buf[0];
  int etype = (int) (hdr->ethertype[0] << 8) + (int) (hdr->ethertype[1]);
  char *end = (char *) &buf[0] + len;
  char *msg = (char *) &buf[0] + sizeof(mrp_ethernet_hdr) + sizeof(mrp_header);
  avb_status_t status = AVB_NO_STATUS;
  int invalid_message = 0;
  //  simple_printf("mrp packet %u\n", len);
  if (etype == AVB_SRP_ETHERTYPE ||
      etype == AVB_MMRP_ETHERTYPE ||
      etype == AVB_MVRP_ETHERTYPE) {
    while (!invalid_message && 
           msg < end && 
           (*msg != 0 || *(msg+1) != 0)) {      
      mrp_msg_header *hdr = (mrp_msg_header *) &msg[0];     
      unsigned attr_list_len = (hdr->AttributeListLength[0]<<8) + 
                                hdr->AttributeListLength[1];
      unsigned first_value_len = hdr->AttributeLength;
      char *next_msg = msg + sizeof(mrp_msg_header) + attr_list_len;
      int attr_type = decode_attr_type(etype, hdr->AttributeType);
      msg = msg + sizeof(mrp_msg_header);

      if (attr_type==-1)
        invalid_message = 1;
      
      while (!invalid_message && 
             msg < next_msg && 
             (*msg != 0 || *(msg+1) != 0)) {
        mrp_vector_header *vector_hdr = (mrp_vector_header *) msg;
        char *first_value = msg + sizeof(mrp_vector_header);
        int numvalues = 
          ((vector_hdr->LeaveAllEventNumberOfValuesHigh & 0x1f)<<8) +
          (vector_hdr->NumberOfValuesLow);
        int leave_all = (vector_hdr->LeaveAllEventNumberOfValuesHigh & 0xe0)>>5;
        int threepacked_len = (numvalues+2)/3;
        int fourpacked_len = 
          has_fourpacked_events(attr_type)?(numvalues+3)/4:0;
        int len = sizeof(mrp_vector_header) + first_value_len + threepacked_len + fourpacked_len;

        
        if (leave_all) {
          attribute_type_event(attr_type, MRP_EVENT_RECEIVE_LEAVE_ALL);
        }
        
        for (int i=0;i<numvalues && !invalid_message;i++) {       
          int three_packed_event = 
            decode_threepacked(*(first_value + first_value_len + i/3), i%3);
          int four_packed_event = 
            has_fourpacked_events(attr_type) ?
            decode_fourpacked(*(first_value + first_value_len + threepacked_len + i/4),
                              i%4):
            0;

          process(attr_type, first_value, i, 
                  three_packed_event, four_packed_event);

          for (int j=0;j<MRP_MAX_ATTRS;j++) {
            if (msg_match(attr_type, &attrs[j], first_value, i, 
                          three_packed_event,
                          four_packed_event)) {
              mrp_in(three_packed_event, &attrs[j]);
            }
          }
        }
        msg = msg + len;
      }
      msg = next_msg;
    }
  }
 
  return status;
}

