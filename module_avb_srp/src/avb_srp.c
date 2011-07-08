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
#ifdef AVNU_OBSERVABILITY
#include "avnu_observability.h"
#endif

#define MIN_ETHERNET_FRAME_SIZE 64

static unsigned int failed_streamId[2];

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
  avb_stream_info_t *source_info = (avb_stream_info_t *) attr->attribute_info;
  unsigned long long stream_id=0, my_stream_id=0;
  srp_talker_first_value *first_value = (srp_talker_first_value *) fv;

  my_stream_id = source_info->streamId[0];
  my_stream_id = (my_stream_id << 32) + source_info->streamId[1];

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
  avb_stream_info_t *sink_info = (avb_stream_info_t *) attr->attribute_info;
  unsigned long long stream_id=0, my_stream_id=0;
  srp_listener_first_value *first_value = (srp_listener_first_value *) fv;


  if (four_packed_event != AVB_SRP_FOUR_PACKED_EVENT_READY)
    return 0;

  my_stream_id = sink_info->streamId[0];
  my_stream_id = (my_stream_id << 32) + sink_info->streamId[1];

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
	if (stream == -1u) return;

	get_avb_source_state(stream, &state);
	if (state == AVB_SOURCE_STATE_POTENTIAL) {
		if (four_packed_event == AVB_SRP_FOUR_PACKED_EVENT_READY ||
			four_packed_event == AVB_SRP_FOUR_PACKED_EVENT_READY_FAILED) {
#ifdef SRP_AUTO_TALKER_STREAM_CONTROL
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
	if (stream == -1u) return;

	get_avb_source_state(stream, &state);
	if (state == AVB_SOURCE_STATE_ENABLED) {
#ifdef SRP_AUTO_TALKER_STREAM_CONTROL
		set_avb_source_state(stream, AVB_SOURCE_STATE_POTENTIAL);
#else
#endif
	}
}


void
avb_srp_process_talker(int mrp_attribute_type, char *fv, int num)
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
  

#ifdef AVNU_OBSERVABILITY
  switch (mrp_attribute_type) 
    {
    case AVB_SRP_ATTRIBUTE_TYPE_TALKER_ADVERTISE:              
      avnu_log(AVNU_TESTPOINT_TA,NULL,"");
      break;
    case AVB_SRP_ATTRIBUTE_TYPE_TALKER_FAILED:
      avnu_log(AVNU_TESTPOINT_TAF,NULL,"");
      break;
    }          
#endif

  // Check if we already know about this stream
  for(int i=0;i<AVB_NUM_SINKS;i++) {
    get_avb_sink_id(i, lstreamId);
    if (pdu_streamId[0] == lstreamId[0] &&
        pdu_streamId[1] == lstreamId[1]) {
      
      registered = 1;
    }
  }                   

  if (!registered)
#ifndef SRP_VERSION_5
    {
      int vlan = ((int) packet->VlanID[0] << 8) | (packet->VlanID[1] & 0xff);
      avb_add_detected_stream(pdu_streamId, vlan, packet->DestMacAddr, num);
    }
#else
    avb_add_detected_stream(pdu_streamId, 2, packet->DestMacAddr, num);
#endif

  return;
}

void avb_srp_talker_join_ind(mrp_attribute_state *attr, int new)
{
	unsigned stream = avb_get_sink_stream_index_from_pointer(attr->attribute_info);
	if (stream != -1u) {
		// This could be used to report talker advertising instead of the snooping scheme above
	}
}

void avb_srp_talker_leave_ind(mrp_attribute_state *attr)
{
	unsigned stream = avb_get_sink_stream_index_from_pointer(attr->attribute_info);
	if (stream != -1u) {
		// This could be used to report talker advertising instead of the snooping scheme above
	}
}


void avb_srp_get_failed_stream(unsigned int streamId[2]) 
{
  streamId[0] = failed_streamId[0];
  streamId[1] = failed_streamId[1];
  return;
}

static int check_listener_merge(char *buf, 
                                avb_stream_info_t *sink_info)
{
  mrp_vector_header *hdr = (mrp_vector_header *) (buf + sizeof(mrp_msg_header));  
  int num_values = hdr->NumberOfValuesLow;
  unsigned long long stream_id=0, my_stream_id=0;
  srp_listener_first_value *first_value = 
    (srp_listener_first_value *) (buf + sizeof(mrp_msg_header) + sizeof(mrp_vector_header));

  // check if we can merge
  my_stream_id = sink_info->streamId[0];
  my_stream_id = (my_stream_id << 32) + sink_info->streamId[1];

  for (int i=0;i<8;i++) {
    stream_id = (stream_id << 8) + first_value->StreamId[i];
  }
  
  stream_id += num_values;


  if (my_stream_id != stream_id)
    return 0;
  
  return 1;

}


static int merge_listener_message(char *buf,
                                  mrp_attribute_state *st,
                                  int vector)
{
  mrp_msg_header *mrp_hdr = (mrp_msg_header *) buf;
  mrp_vector_header *hdr = 
    (mrp_vector_header *) (buf + sizeof(mrp_msg_header));  
  int merge = 0;
  avb_stream_info_t *sink_info = st->attribute_info;

  int num_values;

  if (mrp_hdr->AttributeType != AVB_SRP_ATTRIBUTE_TYPE_LISTENER)
    return 0;

  num_values = hdr->NumberOfValuesLow;
                           
  if (num_values == 0) 
    merge = 1;
  else 
    merge = check_listener_merge(buf, sink_info);


  if (merge) {
    srp_listener_first_value *first_value = 
      (srp_listener_first_value *) (buf + sizeof(mrp_msg_header) + sizeof(mrp_vector_header));
    unsigned * streamId = sink_info->streamId;

    if (num_values == 0) {
      first_value->StreamId[0] = (unsigned char) (streamId[0] >> 24);
      first_value->StreamId[1] = (unsigned char) (streamId[0] >> 16);
      first_value->StreamId[2] = (unsigned char) (streamId[0] >>  8);
      first_value->StreamId[3] = (unsigned char) (streamId[0] >>  0);
      first_value->StreamId[4] = (unsigned char) (streamId[1] >> 24);
      first_value->StreamId[5] = (unsigned char) (streamId[1] >> 16);
      first_value->StreamId[6] = (unsigned char) (streamId[1] >>  8);
      first_value->StreamId[7] = (unsigned char) (streamId[1] >>  0);
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

static int check_domain_merge(char *buf) {
	// We never both to merge domain attribute together
	return 0;
}

static int merge_domain_message(char *buf,
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
    merge = check_domain_merge(buf);

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


static int check_talker_merge(char *buf, 
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
    my_dest_addr = (my_dest_addr << 8) + source_info->dest[i];
    dest_addr = (dest_addr << 8) + first_value->DestMacAddr[i];
  }
  
  dest_addr += num_values;

  if (dest_addr != my_dest_addr)
    return 0;

  // check if we can merge
  my_stream_id = source_info->stream.streamId[0];
  my_stream_id = (my_stream_id << 32) + source_info->stream.streamId[1];

  for (int i=0;i<8;i++) {
    stream_id = (stream_id << 8) + first_value->StreamId[i];
  }

  stream_id += num_values;

  if (my_stream_id != stream_id)
    return 0;


  vlan = NTOH_U16(first_value->VlanID);
  my_vlan = source_info->stream.vlan;

  if (vlan != my_vlan)
    return 0;
  
  my_framesize = 32 + (source_info->stream.num_channels * 6 * 4);

  framesize = NTOH_U16(first_value->TSpecMaxFrameSize);  

  if (framesize != my_framesize)
    return 0;
  
  
  return 1;

}

static int merge_talker_message(char *buf,
                                mrp_attribute_state *st,
                                int vector)
{
  mrp_msg_header *mrp_hdr = (mrp_msg_header *) buf;
  mrp_vector_header *hdr = 
    (mrp_vector_header *) (buf + sizeof(mrp_msg_header));  
  int merge = 0;
  avb_source_info_t *source_info = st->attribute_info;       
  int num_values;
  int samples_per_packet;


  if (mrp_hdr->AttributeType != AVB_SRP_ATTRIBUTE_TYPE_TALKER_ADVERTISE)
    return 0;

  num_values = hdr->NumberOfValuesLow;
                           
  if (num_values == 0) 
    merge = 1;
  else 
    merge = check_talker_merge(buf, source_info);
  
  

  if (merge) {
    srp_talker_first_value *first_value = 
      (srp_talker_first_value *) (buf + sizeof(mrp_msg_header) + sizeof(mrp_vector_header));
    unsigned int framesize;


    // The SRP layer
   
    if (num_values == 0) {
      for (int i=0;i<6;i++) {
        first_value->DestMacAddr[i] = source_info->dest[i];
      }

      first_value->StreamId[0] = (unsigned char) (source_info->stream.streamId[0] >>  24);
      first_value->StreamId[1] = (unsigned char) (source_info->stream.streamId[0] >>  16);
      first_value->StreamId[2] = (unsigned char) (source_info->stream.streamId[0] >>  8);
      first_value->StreamId[3] = (unsigned char) (source_info->stream.streamId[0] >>  0);
      first_value->StreamId[4] = (unsigned char) (source_info->stream.streamId[1] >>  24);
      first_value->StreamId[5] = (unsigned char) (source_info->stream.streamId[1] >>  16);
      first_value->StreamId[6] = (unsigned char) (source_info->stream.streamId[1] >>  8);
      first_value->StreamId[7] = (unsigned char) (source_info->stream.streamId[1] >>  0);

#ifndef SRP_VERSION_5
      //      printintln(source_info->vlan);
      HTON_U16(first_value->VlanID, source_info->stream.vlan);
#endif
 

      first_value->TSpec = AVB_SRP_TSPEC_PRIORITY_DEFAULT << 5 |
        AVB_SRP_TSPEC_RANK_DEFAULT << 4 |
        AVB_SRP_TSPEC_RESERVED_VALUE;
      
      samples_per_packet = (source_info->stream.rate + (AVB1722_PACKET_RATE-1))/AVB1722_PACKET_RATE;
      framesize = 32 + (source_info->stream.num_channels * samples_per_packet * 4);

  

      HTON_U16(first_value->TSpecMaxFrameSize, framesize);
      HTON_U16(first_value->TSpecMaxIntervalFrames,
               AVB_SRP_MAX_INTERVAL_FRAMES_DEFAULT);
      HTON_U32(first_value->AccumulatedLatency, 
               AVB_SRP_ACCUMULATED_LATENCY_DEFAULT);
    }

  mrp_encode_three_packed_event(buf, vector, st->attribute_type);    

    hdr->NumberOfValuesLow = num_values+1;

  }

  return merge;   
}







int avb_srp_merge_message(char *buf,
                          mrp_attribute_state *st,
                          int vector)
{
  switch (st->attribute_type) {
  case MSRP_TALKER_ADVERTISE:
    return merge_talker_message(buf, st, vector);
    break;
  case MSRP_LISTENER:
    return merge_listener_message(buf, st, vector);
    break;
  case MSRP_DOMAIN_VECTOR:
    return merge_domain_message(buf, st, vector);
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
	avb_stream_info_t *sink_info_a = (avb_stream_info_t *) a->attribute_info;
	avb_stream_info_t *sink_info_b = (avb_stream_info_t *) b->attribute_info;
	unsigned int *sA = sink_info_a->streamId;
	unsigned int *sB = sink_info_b->streamId;
	for (int i=0;i<2;i++) {
		if (sA[i] < sB[i])
			return 1;
		if (sB[i] < sA[i])
			return 0;
	}
	return 0;
}

