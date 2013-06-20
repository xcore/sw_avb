#include <xs1.h>
#include "avb_mrp.h"
#include "avb_srp.h"
#include "avb_stream.h"
#include "avb_mmrp.h"
#include "avb_mvrp.h"
#include "avb_mrp_pdu.h"
#include "avb_mmrp_pdu.h"
#include "avb_mvrp_pdu.h"
#include "avb_srp_pdu.h"
#include "misc_timer.h"
#include "ethernet_tx_client.h"
#include "avb_internal.h"
#include <string.h>
#include <print.h>
#include "simple_printf.h"

/** \file avb_mrp.c
 *  \brief the core of the MRP protocols
 */

#define MAX_MRP_MSG_SIZE (sizeof(mrp_msg_header) + sizeof(srp_talker_first_value) + 1 /* for event vector */ + sizeof(mrp_msg_footer))

// The size of the send buffer
#ifndef MRP_SEND_BUFFER_SIZE
#define MRP_SEND_BUFFER_SIZE (500)
#endif

//! Lengths of the first values for each attribute type
static int first_value_lengths[MRP_NUM_ATTRIBUTE_TYPES] = FIRST_VALUE_LENGTHS;

//!@{
//! \name MAC addresses for the various protocols
#ifdef AVB_INCLUDE_MMRP
static unsigned char mmrp_dest_mac[6] = AVB_MMRP_MACADDR;
#endif
#ifndef AVB_EXCLUDE_MVRP
static unsigned char mvrp_dest_mac[6] = AVB_MVRP_MACADDR;
#endif
static unsigned char srp_dest_mac[6] = AVB_SRP_MACADDR;
//!@}

//! Buffer for constructing MRPDUs.  Note: It doesn't necessarily have to be this big,
//! we could always make it shorter and just send more packets.
static char send_buf[MRP_SEND_BUFFER_SIZE];

//! Array of attribute control structures
static mrp_attribute_state attrs[MRP_MAX_ATTRS];

//! when sorting the attributes, this points to the head of the list.  attributes
//! need to be sorted so that they can be merged into vectors in the MRP messages
static mrp_attribute_state *first_attr = &attrs[0];

//! The end of the under-construction MRP packet
static char *send_ptr= &send_buf[0] + sizeof(mrp_ethernet_hdr) + sizeof(mrp_header);

//! The ethertype of the packet under construction - we could probably eliminate this
//! since the information is in the packet anyway
static int current_etype = 0;

//!@{
//! \name Timers for the MRP state machines
static avb_timer periodic_timer[MRP_NUM_PORTS];
static avb_timer joinTimer[MRP_NUM_PORTS];
static avb_timer leaveall_timer[MRP_NUM_PORTS];
//!@}





static void configure_send_buffer(unsigned char* addr, short etype) {
  mrp_ethernet_hdr* hdr = (mrp_ethernet_hdr *) &send_buf[0];
  memcpy(&hdr->dest_addr, addr, 6);
  hdr->ethertype[0] = (etype >> 8);
  hdr->ethertype[1] = etype & 0xff;
  current_etype = etype;
}


unsigned attribute_length_length(mrp_msg_header* hdr)
{
    return (hdr->AttributeListLength[0]<<8) + hdr->AttributeListLength[1];
}

// some MRP based applications do not have attribute list length
// fields.  we build the packets with these fields present (simpler
// to do) then strip them afterwards.  MVRP and MMRP are two
// protocols that do not contain these fields.
static void strip_attribute_list_length_fields()
{
  if (current_etype != AVB_SRP_ETHERTYPE) {
    char *msg = &send_buf[0]+sizeof(mrp_ethernet_hdr)+sizeof(mrp_header);
    char *end = send_ptr;
    while (msg < end && (msg[0]!=0 || msg[1]!=0)) {
      mrp_msg_header* hdr = (mrp_msg_header*)msg;
      char* next = (char*)(hdr+1) + attribute_length_length(hdr);

      for (char* c=(char*)hdr->AttributeListLength; c<end-2; ++c) *c = *(c+2);

      end -= 2;
      msg = next - 2;
    }
    send_ptr = end;
  }
}

// this forces the sending of the current PDU.  this happens when
// that PDU has had all of the attributes that it is going to get,
// or when adding an attribute has filled the PDU up.
static void force_send(chanend c_tx, int ifnum)
{
  char *buf = &send_buf[0];
  char *ptr = send_ptr;

  // Strip out attribute length fields for MMRP and MVRP
  strip_attribute_list_length_fields();

  if (ptr != buf+sizeof(mrp_ethernet_hdr)+sizeof(mrp_header)) {

  // Check that the buffer is long enough for a valid ethernet packet
    char *end = ptr + 4;
    if (end < buf + 64) end = buf + 64;

    // Pad with zero if necessary
    for (char *p = ptr;p<end;p++) *p = 0;

    // Transmit
    mac_tx(c_tx, (unsigned int *) buf, end - buf, ifnum);
  }
  send_ptr = buf+sizeof(mrp_ethernet_hdr)+sizeof(mrp_header);
  return;
}

// this considers whether the send a PDU after an attribute has been
// added, but does not if other attributes could potentially be added
// to it.
static void send(chanend c_tx, int ifnum)
{
  // Send only when the buffer is full
  if (send_buf + MRP_SEND_BUFFER_SIZE < send_ptr + MAX_MRP_MSG_SIZE + sizeof(mrp_footer)) {
    force_send(c_tx, ifnum);
  }
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
        case AVB_MMRP_MAC_VECTOR_ATTRIBUTE_TYPE:
          return MMRP_MAC_VECTOR;
        }
      break;
    case AVB_MVRP_ETHERTYPE:
      switch (atype) 
        {
        case AVB_MVRP_VID_VECTOR_ATTRIBUTE_TYPE:
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
    return AVB_MMRP_MAC_VECTOR_ATTRIBUTE_TYPE;
    break;
  case MVRP_VID_VECTOR:
    return AVB_MVRP_VID_VECTOR_ATTRIBUTE_TYPE;
    break;
  default:
    return 0;
  }
}

static int has_fourpacked_events(mrp_attribute_type attr) {
  return (attr == MSRP_LISTENER) ? 1 : 0;
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
  unsigned attr_list_length = attribute_length_length(hdr);


  if (shift_required) {
    char *endmark;
    if (send_ptr - vector > 0) 
      memmove(vector+1, vector, send_ptr - vector);
    send_ptr++;
    *vector = 0;
    attr_list_length++;
    hton_16(hdr->AttributeListLength, attr_list_length);           
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
  unsigned attr_list_length = attribute_length_length(hdr);



  if (shift_required)  {
    char *endmark;
    if (send_ptr - vector > 0) 
      memmove(vector+1, vector, send_ptr - vector);
    *vector = 0;
    attr_list_length++;
    send_ptr++;
    hton_16(hdr->AttributeListLength, attr_list_length);
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
  int vector_length = 0;
  int first_value_length =  first_value_lengths[attr];
  int attr_list_length = first_value_length + sizeof(mrp_vector_header)  + vector_length + sizeof(mrp_footer);
  int msg_length = hdr_length + attr_list_length;

  // clear message
  memset((char *)hdr, 0, msg_length);

  // Set the relevant fields
  hdr->AttributeType = encode_attr_type(attr);
  hdr->AttributeLength = first_value_length;
  hton_16(hdr->AttributeListLength, attr_list_length);
  
  vector_hdr->LeaveAllEventNumberOfValuesHigh = leave_all << 5;
  vector_hdr->NumberOfValuesLow = 0;

  send_ptr += msg_length;
}


static int encode_msg(char *msg, mrp_attribute_state* st, int vector, unsigned int port_num)
{
  /*
  simple_printf("st->port_num: %d\n port_num: %d\n st->propagate: %d\n", st->port_num, port_num, st->propagate);
  if (st->attribute_type == MSRP_LISTENER)
  {
    printstrln("LR!!");
  }
  printintln(port_to_transmit);
  */
  
  switch (st->attribute_type) 
  {
    case MSRP_TALKER_ADVERTISE: 
    case MSRP_TALKER_FAILED: 
    case MSRP_LISTENER:
    case MSRP_DOMAIN_VECTOR:             
      return avb_srp_encode_message(msg, st, vector);
      break;
#ifdef AVB_INCLUDE_MMRP
    case MMRP_MAC_VECTOR:
      return avb_mmrp_merge_message(msg, st, vector);
      break;
#endif
#ifndef AVB_EXCLUDE_MVRP
    case MVRP_VID_VECTOR:
      return avb_mvrp_merge_message(msg, st, vector);
      break;
#endif
  }

  return 0;
}

static void doTx(mrp_attribute_state *st,
                 int vector,
                 unsigned int port_num)
{
  chanend c_tx = avb_control_get_mac_tx();
  int merged = 0;
  char *msg = &send_buf[0]+sizeof(mrp_ethernet_hdr)+sizeof(mrp_header);
  char *end = send_ptr;

  while (!merged &&
         msg < end && 
         (*msg != 0 || *(msg+1) != 0)) {      
    mrp_msg_header *hdr = (mrp_msg_header *) &msg[0];

    merged = encode_msg(msg, st, vector, port_num);
    
    msg = msg + sizeof(mrp_msg_header) + attribute_length_length(hdr);
  }   

  if (!merged) {
    int port_to_transmit = st->propagate ? !st->port_num : st->port_num;
    if (port_num == port_to_transmit)
    {
      create_empty_msg(st->attribute_type, 0);
      (void) encode_msg(msg, st, vector, port_num);
    }
  }

  send(c_tx, st->propagate ? !st->port_num : st->port_num);
}

static void mrp_update_state(mrp_event e, mrp_attribute_state *st, int four_packed_event, unsigned int port_num)
{
  // simple_printf("mrp_event: %d\n", e);
#ifdef MRP_FULL_PARTICIPANT
  // Registrar state machine
  switch (e) 
    {
    case MRP_EVENT_BEGIN:
      st->registrar_state = MRP_MT;
      break;
    case MRP_EVENT_RECEIVE_NEW:
      if (st->registrar_state == MRP_LV) 
        stop_avb_timer(&st->leaveTimer);
      st->registrar_state = MRP_IN;
      st->pending_indications |= PENDING_JOIN_NEW;
      // printstrln("Pending join new");
      st->four_vector_parameter = four_packed_event;
      break;
    case MRP_EVENT_RECEIVE_JOININ:
    case MRP_EVENT_RECEIVE_JOINMT:
      if (st->registrar_state == MRP_LV)
        stop_avb_timer(&st->leaveTimer);
      if (st->registrar_state == MRP_MT) {
          st->pending_indications |= PENDING_JOIN;
          st->four_vector_parameter = four_packed_event;
      }
      st->registrar_state = MRP_IN;
      break;
    case MRP_EVENT_RECEIVE_LEAVE:
    case MRP_EVENT_RECEIVE_LEAVE_ALL:
    case MRP_EVENT_TX_LEAVE_ALL:
    case MRP_EVENT_REDECLARE:
      if (st->registrar_state == MRP_IN) {
        start_avb_timer(&st->leaveTimer, MRP_LEAVETIMER_PERIOD_CENTISECONDS);
        st->registrar_state = MRP_LV;
      }
      break;
    case MRP_EVENT_LEAVETIMER:
    case MRP_EVENT_FLUSH:
      if (st->registrar_state == MRP_LV) {
        // Lv
        st->pending_indications |= PENDING_LEAVE;
        st->four_vector_parameter = four_packed_event;
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
          doTx(st, vector, port_num);
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
          doTx(st, vector, port_num);
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
}

void mrp_debug_dump_attrs(void)
{

  printstrln("port_num | type | here | propagate | stream_id");
  printstrln("---------|------|------|-----------|----------");
  for (int i=0;i<MRP_MAX_ATTRS;i++) {

    if (attrs[i].applicant_state != MRP_UNUSED) {
      avb_sink_info_t *sink_info = (avb_sink_info_t *) attrs[i].attribute_info;
      int stream_id[2] = {0, 0};

      if (sink_info != NULL) {
        stream_id[0] = sink_info->reservation.stream_id[0];
        stream_id[1] = sink_info->reservation.stream_id[1];
      }

      simple_printf("%d        | %d    | %d    | %d         | %x:%x\n",
        attrs[i].port_num, attrs[i].attribute_type, attrs[i].here, attrs[i].propagate, stream_id[0], stream_id[1]);

    }
  }
}


void mrp_attribute_init(mrp_attribute_state *st,
                        mrp_attribute_type t,
                        unsigned int port_num,
                        unsigned int here,
                        void *info)
{
  st->attribute_type = t;
  st->attribute_info = info;
  st->port_num = port_num;
  st->propagate = 0;
  st->here = here;
  return;
}


void mrp_mad_begin(mrp_attribute_state *st)
{
#ifdef MRP_FULL_PARTICIPANT
  init_avb_timer(&st->leaveTimer, 1);
#endif
  mrp_update_state(MRP_EVENT_BEGIN, st, 0, st->port_num);
}

void mrp_mad_join(mrp_attribute_state *st, int new)
{
  if (new) {
    mrp_update_state(MRP_EVENT_NEW, st, 0, st->port_num);
  } else {
    mrp_update_state(MRP_EVENT_JOIN, st, 0, st->port_num);
  }
}

void mrp_mad_leave(mrp_attribute_state *st)
{
  mrp_update_state(MRP_EVENT_LV, st, 0, st->port_num);
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

  for (int i=0; i < MRP_NUM_PORTS; i++)
  {
    init_avb_timer(&periodic_timer[i], MRP_PERIODIC_TIMER_MULTIPLIER);
    start_avb_timer(&periodic_timer[i], MRP_PERIODIC_TIMER_PERIOD_CENTISECONDS / MRP_PERIODIC_TIMER_MULTIPLIER);

    init_avb_timer(&joinTimer[i], 1);
    start_avb_timer(&joinTimer[i], MRP_JOINTIMER_PERIOD_CENTISECONDS);


  #ifdef MRP_FULL_PARTICIPANT
    init_avb_timer(&leaveall_timer[i], MRP_LEAVEALL_TIMER_MULTIPLIER);
    start_avb_timer(&leaveall_timer[i], MRP_LEAVEALL_TIMER_PERIOD_CENTISECONDS / MRP_LEAVEALL_TIMER_MULTIPLIER);
  #endif
  }

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
  __builtin_trap();
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

static void global_event(mrp_event e, unsigned int port_num) {
  mrp_attribute_state *attr = first_attr;
  while (attr != NULL) {
    if (attr->applicant_state != MRP_DISABLED &&
        attr->applicant_state != MRP_UNUSED &&
        ((attr->port_num == port_num) || attr->propagate))
    {
      if (e != MRP_EVENT_PERIODIC || attr->attribute_type == MVRP_VID_VECTOR)
      {
        mrp_update_state(e, attr, 0, port_num);
      }
    }
    attr = attr->next;
  }
  
}

static void attribute_type_event(mrp_attribute_type atype, mrp_event e, unsigned int port_num) {
  mrp_attribute_state *attr = first_attr;
  while (attr != NULL) {
    if (attr->applicant_state != MRP_DISABLED && 
        attr->applicant_state != MRP_UNUSED &&
        attr->attribute_type == atype &&
        ((attr->port_num == port_num) || attr->propagate))  {
      mrp_update_state(e, attr, 0, port_num);
    }
    attr = attr->next;
  }
}

static void send_join_indication(mrp_attribute_state *st, int new, int four_packed_event)
{
  switch (st->attribute_type)
  {
  case MSRP_TALKER_ADVERTISE:
    avb_srp_talker_join_ind(st, new);
    break;
  case MSRP_TALKER_FAILED:
    break;
  case MSRP_LISTENER:
    avb_srp_listener_join_ind(st, new, four_packed_event);
    break;
  case MSRP_DOMAIN_VECTOR:
    avb_srp_domain_join_ind(st, new);
    break;
#ifdef AVB_INCLUDE_MMRP
  case MMRP_MAC_VECTOR:
    avb_mmrp_mac_vector_join_ind(st, new);
    break;
#endif
#ifndef AVB_EXCLUDE_MVRP
  case MVRP_VID_VECTOR:
    avb_mvrp_vid_vector_join_ind(st, new);
    break;
#endif
  }
}

static void send_leave_indication(mrp_attribute_state *st, int four_packed_event)
{
  switch (st->attribute_type)
  {
  case MSRP_TALKER_ADVERTISE:
    avb_srp_talker_leave_ind(st);
    break;
  case MSRP_TALKER_FAILED:
    break;
  case MSRP_LISTENER:
    avb_srp_listener_leave_ind(st, four_packed_event);
    break;
  case MSRP_DOMAIN_VECTOR:
    avb_srp_domain_leave_ind(st);
    break;
#ifdef AVB_INCLUDE_MMRP
  case MMRP_MAC_VECTOR:
    avb_mmrp_mac_vector_leave_ind(st);
    break;
#endif
#ifndef AVB_EXCLUDE_MVRP
  case MVRP_VID_VECTOR:
    avb_mvrp_vid_vector_leave_ind(st);
    break;
#endif
  }
}

static int leave_all[MRP_NUM_PORTS];

void mrp_periodic(void)
{
  chanend c_tx = avb_control_get_mac_tx();

  for (int i=0; i < MRP_NUM_PORTS; i++)
  {
    if (avb_timer_expired(&periodic_timer[i]))
    {
      global_event(MRP_EVENT_PERIODIC, i);
      start_avb_timer(&periodic_timer[i], MRP_PERIODIC_TIMER_PERIOD_CENTISECONDS / MRP_PERIODIC_TIMER_MULTIPLIER);
    }

  #ifdef MRP_FULL_PARTICIPANT
    if (avb_timer_expired(&leaveall_timer[i]))
    {
      start_avb_timer(&leaveall_timer[i], MRP_LEAVEALL_TIMER_PERIOD_CENTISECONDS / MRP_LEAVEALL_TIMER_MULTIPLIER);
      leave_all[i] = 1;
      global_event(MRP_EVENT_RECEIVE_LEAVE_ALL, i);
    }
  #endif

    if (avb_timer_expired(&joinTimer[i]))
    {
      mrp_event tx_event = leave_all[i] ? MRP_EVENT_TX_LEAVE_ALL : MRP_EVENT_TX;
      start_avb_timer(&joinTimer[i], MRP_JOINTIMER_PERIOD_CENTISECONDS);
      sort_attrs();

      configure_send_buffer(srp_dest_mac, AVB_SRP_ETHERTYPE);
      if (leave_all[i])
      {
        create_empty_msg(MSRP_TALKER_ADVERTISE, 1);  send(c_tx, i);
        create_empty_msg(MSRP_TALKER_FAILED, 1);  send(c_tx, i);
        create_empty_msg(MSRP_LISTENER, 1);  send(c_tx, i);
        create_empty_msg(MSRP_DOMAIN_VECTOR, 1);  send(c_tx, i);
      }
      attribute_type_event(MSRP_TALKER_ADVERTISE, tx_event, i);
      attribute_type_event(MSRP_LISTENER, tx_event, i);
      attribute_type_event(MSRP_DOMAIN_VECTOR, tx_event, i);
      force_send(c_tx, i);

  #ifdef AVB_INCLUDE_MMRP
      configure_send_buffer(mmrp_dest_mac,AVB_MMRP_ETHERTYPE);
      if (leave_all[i])
      {
        create_empty_msg(MMRP_MAC_VECTOR, 1); send(c_tx, i);
      }
      attribute_type_event(MMRP_MAC_VECTOR, tx_event, i);
      force_send(c_tx, i);
  #endif

  #ifndef AVB_EXCLUDE_MVRP
      configure_send_buffer(mvrp_dest_mac, AVB_MVRP_ETHERTYPE);
      if (leave_all[i])
      {
        create_empty_msg(MVRP_VID_VECTOR, 1); send(c_tx, i);
      }
      attribute_type_event(MVRP_VID_VECTOR, tx_event, i);
      force_send(c_tx, i);
  #endif

      leave_all[i] = 0;
    }

    for (int j=0;j<MRP_MAX_ATTRS;j++)
    {
      if (attrs[j].pending_indications != 0)
      {
        /* 5.2 TODO: Used to fire AVB_SRP_INDICATION here */

        if ((attrs[j].pending_indications & PENDING_JOIN_NEW) != 0)
        {
          send_join_indication(&attrs[j], 1, attrs[j].four_vector_parameter);
        }
        if ((attrs[j].pending_indications & PENDING_JOIN) != 0)
        {
          send_join_indication(&attrs[j], 0, attrs[j].four_vector_parameter);
        }
        if ((attrs[j].pending_indications & PENDING_LEAVE) != 0)
        {
          send_leave_indication(&attrs[j], attrs[j].four_vector_parameter);
        }
        attrs[j].pending_indications = 0;
        attrs[j].four_vector_parameter = 0;
      }
  #ifdef MRP_FULL_PARTICIPANT
      if (avb_timer_expired(&attrs[j].leaveTimer))
      {
        mrp_update_state(MRP_EVENT_LEAVETIMER, &attrs[j], 0, i);
      }
  #endif
    }
  }
  return;
}


static void mrp_in(int three_packed_event, int four_packed_event, mrp_attribute_state *st, unsigned int port_num)
{
  // simple_printf("mrp_in event: %d\n", three_packed_event);
  switch (three_packed_event)
    {
    case MRP_ATTRIBUTE_EVENT_NEW:
      // printstrln("MRP_ATTRIBUTE_EVENT_NEW");
      mrp_update_state(MRP_EVENT_RECEIVE_NEW, st, four_packed_event, port_num);
      break;
    case MRP_ATTRIBUTE_EVENT_JOININ:
      mrp_update_state(MRP_EVENT_RECEIVE_JOININ, st, four_packed_event, port_num);
      break;
    case MRP_ATTRIBUTE_EVENT_IN:
      mrp_update_state(MRP_EVENT_RECEIVE_IN, st, four_packed_event, port_num);
      break;
    case MRP_ATTRIBUTE_EVENT_JOINMT:
      mrp_update_state(MRP_EVENT_RECEIVE_JOINMT, st, four_packed_event, port_num);
      break;
    case MRP_ATTRIBUTE_EVENT_MT:
      mrp_update_state(MRP_EVENT_RECEIVE_MT, st, four_packed_event, port_num);
      break;
    case MRP_ATTRIBUTE_EVENT_LV:   
      mrp_update_state(MRP_EVENT_RECEIVE_LEAVE, st, four_packed_event, port_num);
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

int mrp_match_multiple_attrs_by_stream_and_type(mrp_attribute_state *attr)
{
  int matches = 0;

  for (int j=0;j<MRP_MAX_ATTRS;j++) {
    if (attr->applicant_state == MRP_UNUSED) {
      continue;
    }
    if (attr->attribute_type == attrs[j].attribute_type)
    {
      avb_sink_info_t *sink_info = (avb_sink_info_t *) attr->attribute_info;
      avb_source_info_t *source_info = (avb_source_info_t *) attrs[j].attribute_info;

      if (sink_info == NULL || source_info == NULL) continue;

      if (sink_info->reservation.stream_id[0] == source_info->reservation.stream_id[0] && 
          sink_info->reservation.stream_id[1] == source_info->reservation.stream_id[1])
      {
        matches++;
        if (matches == 2)
        {
          return 1;
        }
      }   
    }
  }
  return 0;
}

int mrp_match_attribute_by_stream_id(mrp_attribute_state *attr)
{
  for (int j=0;j<MRP_MAX_ATTRS;j++) {
    if (attr->applicant_state == MRP_UNUSED) {
      continue;
    }
    if ((attr->attribute_type == MSRP_TALKER_ADVERTISE && attrs[j].attribute_type == MSRP_LISTENER) || 
        (attr->attribute_type == MSRP_LISTENER && attrs[j].attribute_type == MSRP_TALKER_ADVERTISE))
    {
      avb_sink_info_t *sink_info = (avb_sink_info_t *) attr->attribute_info;
      avb_source_info_t *source_info = (avb_source_info_t *) attrs[j].attribute_info;

      if (sink_info == NULL || source_info == NULL) continue;

      simple_printf("Compare %x:%x to %x:%x\n", sink_info->reservation.stream_id[0], sink_info->reservation.stream_id[1],
        source_info->reservation.stream_id[0], source_info->reservation.stream_id[1]);

      // Don't match the local attributes
      // if (attr->here && attrs[j].here) continue;

      if (attr->port_num == attrs[j].port_num) continue;


      if (sink_info->reservation.stream_id[0] == source_info->reservation.stream_id[0] && 
          sink_info->reservation.stream_id[1] == source_info->reservation.stream_id[1])
      {
        printstrln("MATCHED ATTRIBUTE BY STREAM ID");
        return 1;
      }   
    }
  }
  return 0;
}

static int match_attribute_of_same_type(mrp_attribute_type attr_type, 
              mrp_attribute_state *attr, 
              char *msg, 
              int i, 
              int three_packed_event,
              int four_packed_event,
              unsigned int port_num)
{
  /*
  if (attr->applicant_state == MRP_UNUSED ||
      attr->applicant_state == MRP_DISABLED)
    return 0;
  */
  if (attr->applicant_state == MRP_UNUSED)
    return 0;

  if (attr->port_num != port_num)
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
#ifdef AVB_INCLUDE_MMRP
  case MMRP_MAC_VECTOR:
    return avb_mmrp_match_mac_vector(attr, msg, i);
#endif
#ifndef AVB_EXCLUDE_MVRP
  case MVRP_VID_VECTOR:
    return avb_mvrp_match_vid_vector(attr, msg, i);
#endif
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

void avb_mrp_process_packet(unsigned char buf[], int etype, int len, unsigned int port_num)
{
  char *end = (char *) &buf[0] + len;
  char *msg = (char *) &buf[0] + sizeof(mrp_header);

  while (msg < end && (msg[0]!=0 || msg[1]!=0))
  {
    mrp_msg_header *hdr = (mrp_msg_header *) &msg[0];     

    unsigned first_value_len = hdr->AttributeLength;
    int attr_type = decode_attr_type(etype, hdr->AttributeType);
    if (attr_type==-1) return;

    msg = msg + sizeof(mrp_msg_header);

    // non-SRP headers don't contain the AttributeListLength
    if (etype != AVB_SRP_ETHERTYPE) msg -= 2;
    
    while (msg < end && (msg[0]!=0 || msg[1]!=0))
    {
      mrp_vector_header *vector_hdr = (mrp_vector_header *) msg;
      char *first_value = msg + sizeof(mrp_vector_header);
      int numvalues = 
        ((vector_hdr->LeaveAllEventNumberOfValuesHigh & 0x1f)<<8) +
        (vector_hdr->NumberOfValuesLow);
      int leave_all = (vector_hdr->LeaveAllEventNumberOfValuesHigh & 0xe0)>>5;
      int threepacked_len = (numvalues+2)/3;
      int fourpacked_len = has_fourpacked_events(attr_type)?(numvalues+3)/4:0;
      int len = sizeof(mrp_vector_header) + first_value_len + threepacked_len + fourpacked_len;
      
      // Check to see that it isn't asking us to overrun the buffer
      if (msg + len > end) return;

      if (leave_all)
      {
        attribute_type_event(attr_type, MRP_EVENT_RECEIVE_LEAVE_ALL, port_num);
      }
      
      for (int i=0;i<numvalues;i++)
      {
        int matched_attribute = 0;
        // Get the three packed data out of the vector
        int three_packed_event = decode_threepacked(*(first_value + first_value_len + i/3), i%3);

        // Get the four packed data out of the vector
        int four_packed_event = has_fourpacked_events(attr_type) ?
          decode_fourpacked(*(first_value + first_value_len + threepacked_len + i/4),i%4) : 0;

        if (MRP_DEBUG_ATTR_INGRESS)
        {
          if (attr_type == MSRP_LISTENER) simple_printf("Port %d in: MSRP_LISTENER\n", port_num);
          if (attr_type == MSRP_TALKER_ADVERTISE) simple_printf("Port %d in: MSRP_TALKER_ADVERTISE\n", port_num);
        }

        // This allows the application state machines to respond to the message
        for (int j=0;j<MRP_MAX_ATTRS;j++)
        {
          // Attempt to match to this endpoint's attributes
          if (match_attribute_of_same_type(attr_type, &attrs[j], first_value, i, three_packed_event, four_packed_event, port_num))
          {
            matched_attribute = 1;
            mrp_in(three_packed_event, four_packed_event, &attrs[j], port_num);
          }
        }

        if (!matched_attribute && !leave_all)
        {
            if (attr_type == MSRP_TALKER_ADVERTISE ||
                attr_type == MSRP_TALKER_FAILED ||
                attr_type == MSRP_LISTENER)
            {
              avb_srp_info_t *stream_data;
              avb_srp_process_attribute(attr_type, first_value, i, &stream_data);
              // if (avb_srp_process_attribute(attr_type, first_value, i, &stream_data))
              if (1)
              {
                mrp_attribute_state *st = mrp_get_attr();
                mrp_attribute_init(st, attr_type, port_num, 0, stream_data);
                simple_printf("mrp_attribute_init: %d, %d, STREAM_ID[0]: %x\n", attr_type, port_num, stream_data->stream_id[0]);

                mrp_in(three_packed_event, four_packed_event, st, port_num);
                // simple_printf("STATE AFTER IN: %d\n", st->applicant_state); 
              }
              else
              {
                simple_printf("Attr didn't match: %d\n", attr_type);
              }
            }
        }

        // This allows various modules to snoop on the individual message
        // process(attr_type, first_value, i, three_packed_event, four_packed_event);

      }
      msg = msg + len;
    }
    msg += 2;
  }
 
  return;
}

