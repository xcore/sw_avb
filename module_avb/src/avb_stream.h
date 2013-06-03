#ifndef __avb_stream_h__
#define __avb_stream_h__

#include "avb_1722_listener.h"
#include "avb_1722_talker.h"

struct mrp_attribute_state;

typedef struct avb_stream_info_t
{
	unsigned int streamId[2];
	int state;
	struct mrp_attribute_state* srp_talker_attr;
	struct mrp_attribute_state* srp_talker_failed_attr;
	struct mrp_attribute_state* srp_listener_attr;
	char core_id;
	char local_id;
	char num_channels;
	char format;
	int rate;
	char sync;
	char vlan;
	short flags;
} avb_stream_info_t;

typedef struct avb_source_info_t
{
	avb_stream_info_t stream;
	int talker_ctl;
	unsigned char dest[6];
	int presentation;
	int map[AVB_MAX_CHANNELS_PER_TALKER_STREAM];
} avb_source_info_t;


typedef struct avb_sink_info_t
{
	avb_stream_info_t stream;
	int listener_ctl;
	unsigned char addr[6];
	int map[AVB_MAX_CHANNELS_PER_LISTENER_STREAM];
} avb_sink_info_t;


#endif // __avb_stream_h__
