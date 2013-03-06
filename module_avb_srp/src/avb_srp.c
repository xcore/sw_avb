#include <xclib.h>
#include <string.h>
#include "avb.h"
#include "avb_conf.h"
#include "avb_srp.h"
#include "avb_mrp_pdu.h"
#include "avb_srp_pdu.h"
#include "avb_stream.h"
#include "avb_1722_talker.h"
#include <print.h>
#include "avb_stream_detect.h"
#include "avb_control_types.h"
#include "avb_internal.h"
#include "stdlib.h"
#include "simple_printf.h"

#define AVB_1722_PLUS_SIP_HEADER_SIZE (32)

static unsigned int failed_streamId[2];

#ifndef AVB_STREAM_DETECT_HISTORY_SIZE
#define AVB_STREAM_DETECT_HISTORY_SIZE 10
#endif

static avb_srp_info_t stream_history[AVB_STREAM_DETECT_HISTORY_SIZE];
static int rdPtr=0;
static int wrPtr = 0;

int avb_srp_match_listener_to_talker_stream_id(unsigned stream_id[2], avb_srp_info_t **stream)
{
  for(int i=0;i<AVB_STREAM_DETECT_HISTORY_SIZE;i++)
    if (stream_id[0] == stream_history[i].stream_id[0] &&
        stream_id[1] == stream_history[i].stream_id[1]) {
      *stream = &stream_history[i];
    return 1;
    }

  return 0;
}

// FIXME: Rename me?
int avb_add_detected_stream(srp_talker_first_value *fv,
                            unsigned stream_id[2],
                             int addr_offset,
                             avb_srp_info_t **stream)
{  
  if (AVB_STREAM_DETECT_HISTORY_SIZE == 0)
    return 0;

  // FIXME: This kind of search should use a linked list when list is big?
  for(int i=0;i<AVB_STREAM_DETECT_HISTORY_SIZE;i++)
    if (stream_id[0] == stream_history[i].stream_id[0] &&
        stream_id[1] == stream_history[i].stream_id[1]) {
      return 0;
    }

  {
    // FIXME: Should be a linked list instead of FIFO
    int new_wrPtr = wrPtr + 1;
    unsigned long long x;

    if (new_wrPtr==AVB_STREAM_DETECT_HISTORY_SIZE)
      new_wrPtr = 0;
    if (new_wrPtr==rdPtr) {
      // fifo is full, drop oldest
      rdPtr++;
      if (rdPtr==AVB_STREAM_DETECT_HISTORY_SIZE)
        rdPtr = 0;
    }
    
    stream_history[wrPtr].stream_id[0] = stream_id[0];
    stream_history[wrPtr].stream_id[1] = stream_id[1];
    stream_history[wrPtr].vlan_id = ntoh_16(fv->VlanID);
      
    for(int i=0;i<6;i++) 
      x = (x<<8) + fv->DestMacAddr[i];

    x += addr_offset;

    memcpy(stream_history[wrPtr].dest_mac_addr, &x, 6);
    stream_history[wrPtr].tspec_max_frame_size = ntoh_16(fv->TSpecMaxFrameSize);
    stream_history[wrPtr].tspec_max_interval = ntoh_16(fv->TSpecMaxIntervalFrames);
    stream_history[wrPtr].tspec = fv->TSpec;
    stream_history[wrPtr].accumulated_latency = ntoh_32(fv->AccumulatedLatency);

    simple_printf("Added stream:\n ID: %x%x\n \
                  DA: %x:%x:%x:%x:%x:%x\n \
                  max size: %d\n \
                  interval: %d\n",
                  stream_id[0], stream_id[1],
                  stream_history[wrPtr].dest_mac_addr[0], stream_history[wrPtr].dest_mac_addr[1], stream_history[wrPtr].dest_mac_addr[2],
                  stream_history[wrPtr].dest_mac_addr[3], stream_history[wrPtr].dest_mac_addr[4], stream_history[wrPtr].dest_mac_addr[5],
                  stream_history[wrPtr].tspec_max_frame_size,
                  stream_history[wrPtr].tspec_max_interval
                  );

    *stream = &stream_history[wrPtr];

    /*
    for(int i=5;i>=0;i--) {
      stream_history[wrPtr].addr[i] = x & 0xff;
      x>>=8;
    }
    */
      
    wrPtr = new_wrPtr;
  }
  
  return 1;
}

/*
int avb_check_for_new_stream(unsigned stream_id[2], unsigned *vlan,
                             unsigned char addr[6])
{
  if (AVB_STREAM_DETECT_HISTORY_SIZE == 0)
    return 0;

  if (rdPtr != wrPtr) {
    stream_id[0] = stream_history[rdPtr].stream_id[0];
    stream_id[1] = stream_history[rdPtr].stream_id[1];
    *vlan = stream_history[rdPtr].vlan;
    for (int i=0;i<6;i++)
      addr[i] = stream_history[rdPtr].addr[i];
    rdPtr++;
    if (rdPtr==AVB_STREAM_DETECT_HISTORY_SIZE)
      rdPtr = 0;    
    return 1;
  }
  return 0;
}
*/

static void avb_srp_map_join(mrp_attribute_state *attr, int new)
{
  printstrln("MAD_Join.indication");
  // Attribute propagation:
  attr->port_num = !(attr->port_num); // Propagate to other port
  mrp_mad_join(attr, new);
}

static void avb_srp_map_leave(mrp_attribute_state *attr)
{
  printstrln("MAD_Leave.indication");
  // Attribute propagation:
  attr->port_num = !(attr->port_num); // Propagate to other port
  mrp_mad_leave(attr);
}

static unsigned avb_srp_calculate_max_framesize(avb_source_info_t *source_info)
{
#if defined(AVB_1722_FORMAT_61883_6) || defined(AVB_1722_FORMAT_SAF)
	unsigned samples_per_packet = (source_info->stream.rate + (AVB1722_PACKET_RATE-1))/AVB1722_PACKET_RATE;
	return AVB_1722_PLUS_SIP_HEADER_SIZE + (source_info->stream.num_channels * samples_per_packet * 4);
#endif
#if defined(AVB_1722_FORMAT_61883_4)
	return AVB_1722_PLUS_SIP_HEADER_SIZE + (192 * MAX_TS_PACKETS_PER_1722);
#endif
}

int avb_srp_match_talker_failed(mrp_attribute_state *attr,
                                char *msg,
                                int i)
{
  return 0;
}

int avb_srp_match_talker_advertise(mrp_attribute_state *attr,
                                   char *fv,
                                   int i)
{
  avb_source_info_t *source_info = (avb_source_info_t *) attr->attribute_info;
  unsigned long long stream_id=0, my_stream_id=0;
  srp_talker_first_value *first_value = (srp_talker_first_value *) fv;

  my_stream_id = source_info->reservation.stream_id[0];
  my_stream_id = (my_stream_id << 32) + source_info->reservation.stream_id[1];

  for (int i=0;i<8;i++) {
    stream_id = (stream_id << 8) + first_value->StreamId[i];
  }

  stream_id += i;

  return (my_stream_id == stream_id);
}

int avb_srp_match_listener(mrp_attribute_state *attr,
                           char *fv,
                           int i,
                           int four_packed_event)
{
  avb_sink_info_t *sink_info = (avb_sink_info_t *) attr->attribute_info;
  unsigned long long stream_id=0, my_stream_id=0;
  srp_listener_first_value *first_value = (srp_listener_first_value *) fv;

  if (four_packed_event != AVB_SRP_FOUR_PACKED_EVENT_READY)
    return 0;

  my_stream_id = sink_info->reservation.stream_id[0];
  my_stream_id = (my_stream_id << 32) + sink_info->reservation.stream_id[1];

  for (int i=0;i<8;i++) {
    stream_id = (stream_id << 8) + first_value->StreamId[i];
  }
  
  stream_id += i;

  return (my_stream_id == stream_id);
}

int avb_srp_match_domain(mrp_attribute_state *attr,char *fv,int i)
{
	// This returns zero becauase we don't expect to have to merge/aggragate domain messages
	return 0;
}

void avb_srp_listener_join_ind(mrp_attribute_state *attr, int new, int four_packed_event)
{
  enum avb_source_state_t state;
	unsigned stream = avb_get_source_stream_index_from_pointer(attr->attribute_info);

	if (stream == -1u)
  {
    avb_srp_map_join(attr, new);
    return;
  }

	get_avb_source_state(stream, &state);
	if (state == AVB_SOURCE_STATE_POTENTIAL) {
		if (four_packed_event == AVB_SRP_FOUR_PACKED_EVENT_READY ||
			four_packed_event == AVB_SRP_FOUR_PACKED_EVENT_READY_FAILED) {
#if SRP_AUTO_TALKER_STREAM_CONTROL
			set_avb_source_state(stream, AVB_SOURCE_STATE_ENABLED);
#else
#endif
		}
	}
}

void avb_srp_listener_leave_ind(mrp_attribute_state *attr, int four_packed_event)
{
  enum avb_source_state_t state;
	unsigned stream = avb_get_source_stream_index_from_pointer(attr->attribute_info);
	if (stream == -1u)
  {
    avb_srp_map_leave(attr);
    return;
  }

	get_avb_source_state(stream, &state);
	if (state == AVB_SOURCE_STATE_ENABLED) {
#if SRP_AUTO_TALKER_STREAM_CONTROL
		set_avb_source_state(stream, AVB_SOURCE_STATE_POTENTIAL);
#else
#endif
	}
}


int avb_srp_process_attribute(int mrp_attribute_type, char *fv, int num, avb_srp_info_t **stream)
{
  // place the streamId in the failed_streamId global in
  // case it is a failed message
  srp_talker_first_value *packet = (srp_talker_first_value *) fv;
  unsigned int *pdu_streamId = &failed_streamId[0];
  int registered = 0;

  unsigned int lstreamId[2];
  unsigned long long streamId;

  for (int i=0;i<8;i++)
    streamId = (streamId << 8) + packet->StreamId[i];

  streamId += num;

  pdu_streamId[0] = streamId >> 32;
  pdu_streamId[1] = (unsigned) streamId;

  switch (mrp_attribute_type)
  {
    case MSRP_TALKER_ADVERTISE:
    case MSRP_TALKER_FAILED:
    {
      // Returns 1 if not found and added --> add new attribute
      // Return 0 if found and not added
      return avb_add_detected_stream(packet, pdu_streamId, num, stream);
    }
    case MSRP_LISTENER:
    {
      // Returns 1 if found --> add new attribute
      // Returns 0 if not found
      return avb_srp_match_listener_to_talker_stream_id(pdu_streamId, stream);
    }
  }

}

void avb_srp_talker_join_ind(mrp_attribute_state *attr, int new)
{
	unsigned stream = avb_get_sink_stream_index_from_pointer(attr->attribute_info);

	if (stream != -1u) {
		// This could be used to report talker advertising instead of the snooping scheme above
	}
  else
  {
    avb_srp_map_join(attr, new);
  }
}

void avb_srp_talker_leave_ind(mrp_attribute_state *attr)
{
	unsigned stream = avb_get_sink_stream_index_from_pointer(attr->attribute_info);
	if (stream != -1u) {
		// This could be used to report talker advertising instead of the snooping scheme above
	}
  else
  {
    avb_srp_map_leave(attr);
  }
}


void avb_srp_get_failed_stream(unsigned int streamId[2]) 
{
  streamId[0] = failed_streamId[0];
  streamId[1] = failed_streamId[1];
  return;
}

static int check_listener_firstvalue_merge(char *buf, 
                                avb_sink_info_t *sink_info)
{
  mrp_vector_header *hdr = (mrp_vector_header *) (buf + sizeof(mrp_msg_header));  
  int num_values = hdr->NumberOfValuesLow;
  unsigned long long stream_id=0, my_stream_id=0;
  srp_listener_first_value *first_value = 
    (srp_listener_first_value *) (buf + sizeof(mrp_msg_header) + sizeof(mrp_vector_header));

  // check if we can merge
  my_stream_id = sink_info->reservation.stream_id[0];
  my_stream_id = (my_stream_id << 32) + sink_info->reservation.stream_id[1];

  for (int i=0;i<8;i++) {
    stream_id = (stream_id << 8) + first_value->StreamId[i];
  }
  
  stream_id += num_values;


  if (my_stream_id != stream_id)
    return 0;
  
  return 1;

}


static int encode_listener_message(char *buf,
                                  mrp_attribute_state *st,
                                  int vector)
{
  mrp_msg_header *mrp_hdr = (mrp_msg_header *) buf;
  mrp_vector_header *hdr = 
    (mrp_vector_header *) (buf + sizeof(mrp_msg_header));  
  int merge = 0;
  avb_sink_info_t *sink_info = st->attribute_info;

  int num_values;

  if (mrp_hdr->AttributeType != AVB_SRP_ATTRIBUTE_TYPE_LISTENER)
    return 0;

  num_values = hdr->NumberOfValuesLow;
                           
  if (num_values == 0) 
    merge = 1;
  else 
    merge = check_listener_firstvalue_merge(buf, sink_info);


  if (merge) {
    srp_listener_first_value *first_value = 
      (srp_listener_first_value *) (buf + sizeof(mrp_msg_header) + sizeof(mrp_vector_header));
    unsigned *streamId = sink_info->reservation.stream_id;
    unsigned int streamid;

    if (num_values == 0) {
      streamid = byterev(streamId[0]);
      memcpy(&first_value->StreamId[0], &streamid, 4);
      streamid = byterev(streamId[1]);
      memcpy(&first_value->StreamId[4], &streamid, 4);
    }
    
    mrp_encode_three_packed_event(buf, vector, st->attribute_type);    
    mrp_encode_four_packed_event(buf, AVB_SRP_FOUR_PACKED_EVENT_READY, st->attribute_type);

    hdr->NumberOfValuesLow = num_values+1;    
    
  }

  return merge;
}

void avb_srp_domain_join_ind(mrp_attribute_state *attr, int new)
{
	//printstr("SRP Domain join ind\n");
}

void avb_srp_domain_leave_ind(mrp_attribute_state *attr)
{
	//printstr("SRP domain leave ind\n");
}

static int check_domain_firstvalue_merge(char *buf) {
	// We never both to merge domain attribute together
	return 0;
}

static int encode_domain_message(char *buf,
                                mrp_attribute_state *st,
                                int vector)
{
  mrp_msg_header *mrp_hdr = (mrp_msg_header *) buf;
  mrp_vector_header *hdr = 
    (mrp_vector_header *) (buf + sizeof(mrp_msg_header));  
  int merge = 0;
  int num_values;


  if (mrp_hdr->AttributeType != AVB_SRP_ATTRIBUTE_TYPE_DOMAIN)
    return 0;


  num_values = hdr->NumberOfValuesLow;
                           
  if (num_values == 0) 
    merge = 1;
  else 
    merge = check_domain_firstvalue_merge(buf);

  if (merge) { 
    srp_domain_first_value *first_value = 
      (srp_domain_first_value *) (buf + sizeof(mrp_msg_header) + sizeof(mrp_vector_header));    

    first_value->SRclassID = AVB_SRP_SRCLASS_DEFAULT;
    first_value->SRclassPriority = AVB_SRP_TSPEC_PRIORITY_DEFAULT;
    first_value->SRclassVID[0] = (AVB_DEFAULT_VLAN>>8)&0xff;
    first_value->SRclassVID[1] = (AVB_DEFAULT_VLAN&0xff);

    mrp_encode_three_packed_event(buf, vector, st->attribute_type);    

    hdr->NumberOfValuesLow = num_values+1;
  }    
  
  return merge;
}


static int check_talker_firstvalue_merge(char *buf, 
                              avb_source_info_t *source_info)
{
  mrp_vector_header *hdr = (mrp_vector_header *) (buf + sizeof(mrp_msg_header));  
  int num_values = hdr->NumberOfValuesLow;
  unsigned long long stream_id=0, my_stream_id=0;
  unsigned long long dest_addr=0, my_dest_addr=0;
  int framesize=0, my_framesize=0;
  int vlan, my_vlan;
  srp_talker_first_value *first_value = 
    (srp_talker_first_value *) (buf + sizeof(mrp_msg_header) + sizeof(mrp_vector_header));

  // check if we can merge

  for (int i=0;i<6;i++) {
    my_dest_addr = (my_dest_addr << 8) + source_info->reservation.dest_mac_addr[i];
    dest_addr = (dest_addr << 8) + first_value->DestMacAddr[i];
  }
  
  dest_addr += num_values;

  if (dest_addr != my_dest_addr)
    return 0;

  // check if we can merge
  my_stream_id = source_info->reservation.stream_id[0];
  my_stream_id = (my_stream_id << 32) + source_info->reservation.stream_id[1];

  for (int i=0;i<8;i++) {
    stream_id = (stream_id << 8) + first_value->StreamId[i];
  }

  stream_id += num_values;

  if (my_stream_id != stream_id)
    return 0;


  vlan = ntoh_16(first_value->VlanID);
  my_vlan = source_info->reservation.vlan_id;

  if (vlan != my_vlan)
    return 0;
  
  my_framesize = avb_srp_calculate_max_framesize(source_info);
  framesize = ntoh_16(first_value->TSpecMaxFrameSize);  

  if (framesize != my_framesize)
    return 0;
  
  
  return 1;

}

static int encode_talker_message(char *buf,
                                mrp_attribute_state *st,
                                int vector)
{
  mrp_msg_header *mrp_hdr = (mrp_msg_header *) buf;
  mrp_vector_header *hdr = 
    (mrp_vector_header *) (buf + sizeof(mrp_msg_header));  
  int merge = 0;
  avb_source_info_t *source_info = st->attribute_info;
  avb_srp_info_t *attribute_info;
  int num_values;

  if (mrp_hdr->AttributeType != AVB_SRP_ATTRIBUTE_TYPE_TALKER_ADVERTISE)
    return 0;

  if (st->here)
    attribute_info = &source_info->reservation;
  else
    attribute_info = st->attribute_info;

  num_values = hdr->NumberOfValuesLow;
                           
  if (num_values == 0) 
    merge = 1;
  else 
    merge = check_talker_firstvalue_merge(buf, source_info);
  
  

  if (merge) {
    srp_talker_first_value *first_value = 
      (srp_talker_first_value *) (buf + sizeof(mrp_msg_header) + sizeof(mrp_vector_header));

    // The SRP layer
   
    if (num_values == 0) {
      unsigned int streamid;
      for (int i=0;i<6;i++) {
        first_value->DestMacAddr[i] = attribute_info->dest_mac_addr[i];
      }

      streamid = byterev(attribute_info->stream_id[0]);
      memcpy(&first_value->StreamId[0], &streamid, 4);
      streamid = byterev(attribute_info->stream_id[1]);
      memcpy(&first_value->StreamId[4], &streamid, 4);

      hton_16(first_value->VlanID, attribute_info->vlan_id);

      char here = st->here;

      // TODO: Could improve this by storing these values for the endpoint in the avb_srp_info_t 
      // fields. We don't need to calculate everytime because the parameters cannot change without
      // tearing down the stream
      {
        short tspec_max_frame_size;
        short tspec_max_interval;
        unsigned char tspec;
        unsigned accumulated_latency;

        tspec_max_frame_size = here ? avb_srp_calculate_max_framesize(attribute_info) : attribute_info->tspec_max_frame_size;
        tspec_max_interval = here ? AVB_SRP_MAX_INTERVAL_FRAMES_DEFAULT : attribute_info->tspec_max_interval;
        tspec = here ? (AVB_SRP_TSPEC_PRIORITY_DEFAULT << 5 |
          AVB_SRP_TSPEC_RANK_DEFAULT << 4 |
          AVB_SRP_TSPEC_RESERVED_VALUE) : attribute_info->tspec_max_interval;
        accumulated_latency = here ? AVB_SRP_ACCUMULATED_LATENCY_DEFAULT : attribute_info->accumulated_latency;

        first_value->TSpec = tspec;
        hton_16(first_value->TSpecMaxFrameSize, tspec_max_frame_size);
        hton_16(first_value->TSpecMaxIntervalFrames,
                   tspec_max_interval);
        hton_32(first_value->AccumulatedLatency, 
                   accumulated_latency);

      }

    }

    mrp_encode_three_packed_event(buf, vector, st->attribute_type);    

    hdr->NumberOfValuesLow = num_values+1;

  }

  return merge;   
}







int avb_srp_encode_message(char *buf,
                          mrp_attribute_state *st,
                          int vector)
{
  switch (st->attribute_type) {
  case MSRP_TALKER_ADVERTISE:
    return encode_talker_message(buf, st, vector);
    break;
  case MSRP_LISTENER:
    return encode_listener_message(buf, st, vector);
    break;
  case MSRP_DOMAIN_VECTOR:
    return encode_domain_message(buf, st, vector);
    break;

  default:
    break;
  }
  return 0;
}


int avb_srp_compare_talker_attributes(mrp_attribute_state *a,
                                      mrp_attribute_state *b)
{
	avb_stream_info_t *source_info_a = (avb_stream_info_t *) a->attribute_info;
	avb_stream_info_t *source_info_b = (avb_stream_info_t *) b->attribute_info;
	return (source_info_a->local_id < source_info_b->local_id);
}

int avb_srp_compare_listener_attributes(mrp_attribute_state *a,
                                       mrp_attribute_state *b)
{
	avb_sink_info_t *sink_info_a = (avb_sink_info_t *) a->attribute_info;
	avb_sink_info_t *sink_info_b = (avb_sink_info_t *) b->attribute_info;
	unsigned int *sA = sink_info_a->reservation.stream_id;
	unsigned int *sB = sink_info_b->reservation.stream_id;
	for (int i=0;i<2;i++) {
		if (sA[i] < sB[i])
			return 1;
		if (sB[i] < sA[i])
			return 0;
	}
	return 0;
}

