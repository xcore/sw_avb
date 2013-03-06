#ifndef __avb_stream_h__
#define __avb_stream_h__

struct mrp_attribute_state;

typedef struct avb_stream_info_t
{
	int state;
	struct mrp_attribute_state* srp_talker_attr;
	struct mrp_attribute_state* srp_talker_failed_attr;
	struct mrp_attribute_state* srp_listener_attr;
	char tile_id;
	char local_id;
	char num_channels;
	char format;
	int rate;
	char sync;
	short flags;
} avb_stream_info_t;

typedef struct avb_source_info_t
{
	avb_srp_info_t reservation;
	avb_stream_info_t stream;
	int talker_ctl;
	int presentation;
	int map[AVB_MAX_CHANNELS_PER_TALKER_STREAM];
} avb_source_info_t;


typedef struct avb_sink_info_t
{
	avb_srp_info_t reservation;
	avb_stream_info_t stream;
	int listener_ctl;
	int map[AVB_MAX_CHANNELS_PER_LISTENER_STREAM];
} avb_sink_info_t;


#endif // __avb_stream_h__
